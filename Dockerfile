FROM ubuntu:18.04

COPY . /app
WORKDIR /app

RUN apt-get update && apt-get upgrade -y \
    && apt-get install -y \
    build-essential \
    && make BUILD_TYPE=release

CMD [ "/app/build/release/mocc" ]
