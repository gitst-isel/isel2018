# Ejercicio 6.1: Alarma básica

Implementar una alarma con sensores de infrarrojos (PIR) para detectar presencia de intrusos (GPIO12 en D6). Si la alarma está armada, la detección de presencia debe generar una señal de alarma.

La alarma estará armada si hay un 1 en el GPIO 14 (D5) y desarmada en caso contrario.

La señal de alarma será el LED interno, pero se puede conectar un buzzer al GPIO 2 (cuidado con las corrientes máximas que puede dar).
