FROM gcc:4.8
COPY . /usr/src/datahubapi
WORKDIR /usr/src/datahubapi
RUN make clean && make
EXPOSE 8088
CMD ["/usr/src/datahubapi/LdpApiServer -c /usr/src/datahubapi/config/userQuery.conf "]
