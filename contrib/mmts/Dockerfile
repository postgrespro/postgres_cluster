FROM pgproent

RUN mkdir /pg/mmts
COPY ./ /pg/mmts/

RUN export USE_PGXS=1 && \
    cd /pg/mmts && make clean && make install

RUN export USE_PGXS=1 && \
    cd /pg/src/contrib/referee && make clean && make install

# pg_regress client assumes such dir exists on server
RUN cp /pg/src/src/test/regress/*.so /pg/install/lib/postgresql/
USER postgres
ENV PGDATA /pg/data
ENTRYPOINT ["/pg/mmts/tests2/docker-entrypoint.sh"]

EXPOSE 5432
CMD ["postgres"]
