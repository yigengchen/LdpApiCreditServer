FROM gcc:4.8
COPY . /usr/src/datahubapi/src
WORKDIR /usr/src/datahubapi/src
RUN make
CMD ["./config/userQuery.conf "]
