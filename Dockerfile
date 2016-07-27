# vim:set ft=dockerfile:
FROM debian:jessie

# explicitly set user/group IDs
RUN groupadd -r postgres --gid=999 && useradd -r -g postgres --uid=999 postgres

# make the "en_US.UTF-8" locale so postgres will be utf-8 enabled by default
RUN apt-get update && apt-get install -y locales && rm -rf /var/lib/apt/lists/* \
	&& localedef -i en_US -c -f UTF-8 -A /usr/share/locale/locale.alias en_US.UTF-8
ENV LANG en_US.utf8

# postgres build deps
RUN apt-get update && apt-get install -y \
	git \
	make \
	gcc \
	gdb \
	libreadline-dev \
	bison \
	flex \
	zlib1g-dev \
	sudo \
	&& rm -rf /var/lib/apt/lists/*

RUN mkdir /pg && chown postgres:postgres /pg
# We need that to allow editing of /proc/sys/kernel/core_pattern
# from docker-entrypoint.sh
RUN echo "postgres ALL=(ALL:ALL) NOPASSWD:ALL" >> /etc/sudoers

COPY ./ /pg/src
RUN chown -R postgres:postgres /pg/src

USER postgres
ENV CFLAGS -O0
WORKDIR /pg

RUN cd /pg/src && \
	ls -la && \
	whoami && \
	./configure  --enable-cassert --enable-debug --prefix=/pg/install && \
	make -j 4 install

ENV PATH /pg/install/bin:$PATH
ENV PGDATA /pg/data

RUN cd /pg/src/contrib/raftable && make clean && make install
RUN cd /pg/src/contrib/mmts && make clean && make install

ENTRYPOINT ["/pg/src/contrib/mmts/tests2/docker-entrypoint.sh"]

EXPOSE 5432
CMD ["postgres"]
