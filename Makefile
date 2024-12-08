obj-m += pcf8574.o

# Polecenie uruchamiające proces budowania modułu jądra linux
# -C oznacza wykoannie polecenia make w podanej ścieżce (ścieżka do plików środowiska buildowania modułów)
# Zmienna M= wskazuje ścieżkę, gdzie mają zostać umieszczone pliki wynikowe operacji budoania modułu

all:
		make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
		make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
