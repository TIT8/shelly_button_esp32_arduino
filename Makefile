all:
      g++ -std=c++17 shelly_button_esp32.ino -o output

clean:
      $(RM) hello
