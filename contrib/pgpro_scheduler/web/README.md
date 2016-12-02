# Простейшее веб приложение для простомтра активности

Здесь находится пример конфигурации `nginx` и js скрипты для построения 
web сервера, который позволяет просматривать автивность планировщика задач
в какой-то одной базе.

Приложение сделано исключительно для тестов!!!

## Установка

### Сборка nginx 

Для сборки  веб сервера потребуются следующие модули:

* **ngx_postgres** - https://github.com/FRiCKLE/ngx_postgres
* **rds-json-nginx-module** - https://github.com/openresty/rds-json-nginx-module

Загрузите их или склонируйте из GITHUB. 

	$ ./configure --add-module=../rds-json-nginx-module --add-module=../ngx_postgres
	$ make
	$ make install

### Установка файлов

Скопируйте `nginx-conf/nginx.conf` в директорию,  где находятся
конфигурационные  файлы `nginx`. Обычно это `/usr/local/nginx/conf/`.

	$ cp nginx-conf/nginx.conf /usr/local/nginx/conf/

Создайте директорию  `/usr/local/share/pgpro_scheduler/htdocs` и скопируйте в
нее  содержимое директории `htdocs`.

	$ mkdir /usr/local/share/pgpro_scheduler/htdocs
	$ cp -r htdocs/* /usr/local/share/pgpro_scheduler/htdocs

## Настройка 

В конфигурационном файле `nginx` нужно поменять директиву `upstream`, где нужно 
указать правильные реквизиты доступа к postgres.
