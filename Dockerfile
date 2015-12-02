FROM gcc:4.8
COPY . /usr/src/datahubapi
WORKDIR /usr/src/datahubapi
RUN make clean && make
EXPOSE 8088
RUN chmod u+x start.sh
CMD ./start.sh
