FROM gcc:4.8
COPY . /usr/src/datahubapi
WORKDIR /usr/src/datahubapi
RUN make && make install && make clean
EXPOSE 8088
CMD ["./$WORKDIR/config/userQuery.conf "]
