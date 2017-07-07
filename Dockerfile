FROM debian:stable-20170620

COPY . /usr/src/kafkacat

RUN runDeps=''; \
  buildDeps='curl ca-certificates build-essential zlib1g-dev python cmake'; \
  set -ex; \
  apt-get update && apt-get install -y $runDeps $buildDeps --no-install-recommends; \
  rm -rf /var/lib/apt/lists/*; \
  \
  cd /usr/src/kafkacat; \
  ./bootstrap.sh; \
  mv ./kafkacat /usr/local/bin/; \
  \
  rm -rf /usr/src/kafkacat/tmp-bootstrap; \
  apt-get purge -y --auto-remove $buildDeps

ENTRYPOINT ["kafkacat"]
