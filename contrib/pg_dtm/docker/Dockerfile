FROM debian:sid
MAINTAINER Constantin S. Pan <kvapen@gmail.com>

RUN apt-get update && apt-get -y install iptables libpqxx-dev libpq-dev

COPY postgres /postgres
COPY dtmd /dtmd
COPY dtmbench /dtmbench

RUN mkdir /datadir
RUN useradd -m -s /bin/bash pro
RUN chown pro /datadir

USER pro
