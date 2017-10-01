FROM alpine:3.6

RUN apk add --update alpine-sdk bash python cmake; \
    curl https://codeload.github.com/edenhill/kafkacat/tar.gz/master | tar xzf - && cd kafkacat-* && bash ./bootstrap.sh; \
    mv /kafkacat-master/kafkacat /usr/local/bin/; \
    rm -rf /kafkacat-master
ENTRYPOINT ["kafkacat"]