FROM kelvich/postgres_cluster

USER postgres

RUN cd /pg/src/contrib/raftable && make clean && make install

RUN mkdir /pg/mmts
COPY ./ /pg/mmts/

ENV RAFTABLE_PATH /pg/src/contrib/raftable
ENV USE_PGXS 1
ENV PGDATA /pg/data

RUN cd /pg/mmts && make clean && make install

# RUN cd /pg/src/src/test/regress && make && ./pg_regress || true

ENTRYPOINT ["/pg/mmts/tests2/docker-entrypoint.sh"]

EXPOSE 5432
CMD ["postgres"]

