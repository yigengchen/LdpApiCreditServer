FROM gcc:4.8
COPY . /usr/src/myapp
WORKDIR /usr/src/myapp
RUN make clean && make
EXPOSE 8088
CMD ["start.sh","run"]
