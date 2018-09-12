#!/bin/bash
docker build -t=kafkacat:1.3.1 -f build_Dockerfile .
docker cp $(docker create kafkacat:1.3.1):/kafkacat/kafkacat ./kafkacat
echo "built binary ./kafkacat"
