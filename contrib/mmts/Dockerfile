FROM kelvich/postgres_cluster
# RUN sysctl -w kernel.core_pattern=core
RUN cd /pg/src/contrib/raftable && make clean && make install

RUN mkdir /pg/mmts
COPY ./ /pg/mmts/

RUN export RAFTABLE_PATH=/pg/src/contrib/raftable && \
    export USE_PGXS=1 && \
    cd /pg/mmts && make clean && make install

# pg_regress client assumes such dir exists on server

USER postgres
RUN mkdir /pg/src/src/test/regress/results
ENV PGDATA /pg/data
ENTRYPOINT ["/pg/mmts/tests2/docker-entrypoint.sh"]

EXPOSE 5432
CMD ["postgres"]
