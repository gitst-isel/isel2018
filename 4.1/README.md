# Ejercicio 4.1: Lámpara con múltiples pulsadores

Se deseaimplementar el control de una lámpara que tiene múltiples pulsadores, con una plataforma WeMos D1 mini. Para simplificar, utilizaremos el LED integrado en la placa como lámpara, que corresponde con el GPIO 2 cuando está configurado como salida. La única diferencia con el sistema real es que a esta salida iría conectado un relé.

En primer lugar, suponemos que los pulsadores están conectados a GPIOs del mismo dispositivo, configurados como entradas (GPIO5 en D1 y GPIO4 en D2). Debe implementarse un antirrebotes software.
