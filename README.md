# dns-server

# Сборка
mkdir build && cd build
cmake .. && make

# Конфигурация через config.json
port на котором запустится сервер, 0 - случайный

# Запуск
./dns_server
./dns_server path/to/config.json

# Тестирование
dig @localhost -p 5353 example.com
nslookup -port=5353 google.com localhost