ARG BOOTSTRAP
FROM ${BOOTSTRAP:-alpine:3.7} as gentoo
RUN apk --no-cache add gnupg tar wget xz strace

WORKDIR /gentoo

ARG ARCH=amd64
ARG MICROARCH=amd64
ARG SUFFIX=-uclibc-hardened
ARG DIST="http://ftp-osl.osuosl.org/pub/gentoo/releases/${ARCH}/autobuilds"
ARG SIGNING_KEY="0xBB572E0E2D182910"

RUN echo "Building Gentoo Container image for ${ARCH} ${SUFFIX} fetching from ${DIST}" \
	&& wget -O- 'http://ha.pool.sks-keyservers.net/pks/lookup?op=get&options=mr&search='${SIGNING_KEY} | gpg --import \
	&& STAGE3PATH="$(wget -O- "${DIST}/latest-stage3-${MICROARCH}${SUFFIX}.txt" | tail -n 1 | cut -f 1 -d ' ')" \
	&& echo "STAGE3PATH:" $STAGE3PATH \
	&& STAGE3="$(basename ${STAGE3PATH})" \
	&& wget --progress=dot:giga "${DIST}/${STAGE3PATH}" "${DIST}/${STAGE3PATH}.CONTENTS" "${DIST}/${STAGE3PATH}.DIGESTS.asc" \
	&& gpg --verify "${STAGE3}.DIGESTS.asc" \
	&& awk '/# SHA512 HASH/{getline; print}' ${STAGE3}.DIGESTS.asc | sha512sum -c \
	&& tar xpf "${STAGE3}" --xattrs --numeric-owner \
	&& sed -i -e 's/#rc_sys=""/rc_sys="docker"/g' etc/rc.conf \
	&& echo 'UTC' > etc/timezone \
	&& rm ${STAGE3}.DIGESTS.asc ${STAGE3}.CONTENTS ${STAGE3}

FROM gentoo/portage:latest as portage
FROM scratch as builder
COPY --from=gentoo /gentoo/ /
COPY --from=portage /usr/portage /usr/portage

RUN true \
	&& echo 'CONFIG_PROTECT="-*"' >>/etc/portage/make.conf \
	&& echo 'USE="static static-libs"' >>/etc/portage/make.conf \
	&& echo 'FEATURES="-sandbox -ipc-sandbox -network-sandbox -pid-sandbox -usersandbox"' >>/etc/portage/make.conf # can't sandbox in the sandbox \
	&& sed -i '/^\/usr\/lib$/ d; /^\/lib$/ d;' /etc/ld.so.conf

RUN emerge --unmerge openssh ssh \
	&& emerge --autounmask-write --autounmask-continue --tree --verbose --update --deep --newuse \
		--exclude='openssh ssh' \
		--exclude='gzip bzip2 tar xz' \
		--exclude='debianutils patch pinentry' \
		kafkacat meson dev-util/ninja \
		app-arch/zstd app-arch/lz4 dev-libs/cyrus-sasl

COPY . /opt/kafkacat
RUN cd /opt/kafkacat \
	&& meson build --wrap-mode forcefallback -Ddefault_library=static -Dstatic=true \
	&& ninja -C build kafkacat \
	&& ldd build/kafkacat | grep -q 'not.*dynamic'

FROM scratch
USER 1000
ENTRYPOINT ["/kafkacat"]
COPY --from=builder /opt/kafkacat/build/kafkacat /
