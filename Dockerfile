FROM gcc:5.2.0
COPY . /usr/src/datahubapi
WORKDIR /usr/src/datahubapi
RUN make
CMD ["./datahubapi"]
