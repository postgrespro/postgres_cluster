FROM alpine:3.4

RUN apk add --update gcc libc-dev bison flex readline-dev zlib-dev perl make diffutils gdb iproute2 musl-dbg

# there is already accidental postgres user in alpine
# RUN addgroup pg && adduser -h /pg -D -G pg pg
RUN mkdir /pg && chown postgres:postgres pg

ENV LANG en_US.utf8
ENV CFLAGS -O0
ENV PATH /pg/install/bin:$PATH

COPY ./ /pg/src
RUN chown -R postgres:postgres /pg

RUN cd /pg/src && \
	./configure --enable-cassert --enable-debug --prefix=/pg/install && \
	make -j 4 install
