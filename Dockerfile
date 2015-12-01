FROM gcc:4.8
COPY . /usr/src/datahubapi/src
WORKDIR /usr/src/datahubapi/src
RUN make
EXPOSE 8088
CMD ["./usr/src/datahubapi/config/userQuery.conf "]
