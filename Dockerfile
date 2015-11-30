FROM gcc:5.2.0
COPY . /usr/src/datahubapi/src
WORKDIR /usr/src/datahubapi/src
RUN make
CMD ["./datahubapi"]
