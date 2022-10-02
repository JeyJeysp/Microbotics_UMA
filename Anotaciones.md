# Servo Motores
Supondremos que estamos mirando desde el usb hacia nosotros.
+ Pin de la placa PF2 -> Salida P6 del Módulo 1 del PWM -> Motor derecho
+ Pin de la placa PF3 -> Salida P7 del Módulo 1 del PWM -> Motor izquierdo

Variables creadas por nosotros:
+ dutycicle[0] -> referencia al motor derecho
+ dutycicle[1] -> referencia al motor izquierdo

Hemos modificado las variables de 1MS y 2MS para que se ajuste bien la velocidad:
+ 3550 de número de ciclos para 1MS
+ 4200 de número de ciclos para 2MS
