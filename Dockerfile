FROM gcc:4.8
COPY . /usr/src/datahubapi
RUN make
EXPOSE 8088
WORKDIR /usr/src/datahubapi/config
CMD ["./$WORKDIR/userQuery.conf "]
