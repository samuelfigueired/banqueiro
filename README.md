# Algoritmo do Banqueiro (SO)

Implementação do algoritmo do banqueiro em C para Windows, simulando múltiplos clientes e recursos com controle de concorrência via threads.

## Como compilar

Requer MinGW (gcc) instalado no Windows.

```powershell
C:\MinGW\bin\gcc.exe -Wall -Wextra -g3 -finput-charset=UTF-8 -fexec-charset=UTF-8 banqueiro.c -o output\banqueiro.exe
```

## Como executar

Entre na pasta do projeto e rode:

```powershell
output\banqueiro.exe 10 5 7
```

Passe 3 números inteiros (quantidade de cada recurso).

## Arquivos
- `banqueiro.c`: código-fonte principal
- `output/banqueiro.exe`: executável gerado

## Exemplo de uso

```powershell
output\banqueiro.exe 10 5 7
```

## Autor
Samuel Figueiredo

---
Trabalho de Sistemas Operacionais
