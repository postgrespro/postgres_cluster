# vim:set ft=dockerfile:
FROM debian:jessie

# explicitly set user/group IDs
RUN groupadd -r postgres --gid=999 && useradd -r -g postgres --uid=999 postgres

RUN apt-get update && apt-get install -y git && rm -rf /var/lib/apt/lists/*

RUN apt-get update && apt-get install -y \
	make \
	gcc \
	libreadline-dev \
	bison \
	flex \
	zlib1g-dev \
	&& rm -rf /var/lib/apt/lists/*

RUN mkdir /pg
RUN chown postgres:postgres /pg

USER postgres

WORKDIR /pg
ENV CFLAGS -O0
RUN git clone -b master https://github.com/postgrespro/postgres_cluster.git --depth 1

WORKDIR /pg/postgres_cluster
RUN ./configure --enable-cassert --enable-debug --prefix /usr/local
RUN make -j 4
USER root
RUN make install
RUN make -C contrib/raftable install
RUN make -C contrib/mmts install

RUN mkdir -p /var/run/postgresql && chown -R postgres /var/run/postgresql

ENV PATH /usr/local/bin:$PATH
ENV PGDATA /var/run/postgresql/data

COPY docker-entrypoint.sh  /

USER postgres
ENTRYPOINT ["/docker-entrypoint.sh"]

EXPOSE 5432
EXPOSE 5431
CMD ["postgres"]
