FROM gcc:4.8
COPY . /usr/src/datahubapi
WORKDIR /usr/src/datahubapi/config
RUN make
EXPOSE 8088
CMD ["./$WORKDIR/userQuery.conf "]
