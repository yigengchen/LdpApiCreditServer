FROM gcc:4.8
COPY . /usr/src/datahubapi
WORKDIR /usr/src/datahubapi
RUN make clean && make
EXPOSE 8088
CMD ["./$WORKDIR/config/userQuery.conf "]
