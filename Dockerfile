FROM alpine:3.8

COPY . /app
WORKDIR /app

RUN apk add --no-cache \
    bash \
    gcc \
    libc-dev \
    make \
    && make BUILD_TYPE=release

CMD [ "/app/build/release/mocc" ]
