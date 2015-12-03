FROM gcc:4.8
ENV WORKDIR /usr/src/datahubapi
COPY . $WORKDIR
RUN make clean && make
EXPOSE 8088
CMD ["start.sh","run"]
