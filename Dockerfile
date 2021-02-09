FROM alpine:latest

# Iputils fetches non-busybox ping which allows to run it under non-root, see
# https://github.com/gliderlabs/docker-alpine/issues/253
RUN apk add --update gcc libc-dev bison flex readline-dev zlib-dev perl make diffutils gdb iproute2 musl-dbg iputils linux-headers

# Finally, there is no accidental postgres user in alpine, so create one
RUN addgroup postgres && adduser -h /pg -D -G postgres postgres

ENV LANG en_US.utf8
ENV CFLAGS -O0
ENV PATH /pg/install/bin:$PATH

COPY ./ /pg/src

RUN cd /pg/src && \
	CFLAGS=-ggdb3 ./configure --enable-cassert --enable-debug --prefix=/pg/install && \
	make -j 8 install

# Crutch to allow regression test to write there
RUN chown -R postgres:postgres /pg/src/src/test/regress