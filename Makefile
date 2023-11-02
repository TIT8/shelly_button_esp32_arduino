all:
      g++ -std=c++17 hello.cpp -o hello

clean:
      $(RM) hello
