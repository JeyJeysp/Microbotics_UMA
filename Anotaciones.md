# Bloc de notas/dudas/bugs/cambios

## Notas
Componente|Contenido
---|---
**Servo Motores** | <ul>Supondremos que estamos mirando hacia el USB:<p><li>Pin de la placa PF2 -> Salida P6 del Módulo 1 del PWM -> Motor derecho.<li>Pin de la placa PF3 -> Salida P7 del Módulo 1 del PWM -> Motor izquierdo. <p>Variables creadas por nosotros:<li>dutycicle[0] -> referencia al motor derecho.<li>dutycicle[1] -> referencia al motor izquierdo.<p>Hemos modificado las variables de 1MS y 2MS para que se ajuste bien la velocidad:<li>3550 de número de ciclos para 1MS.<li>4200 de número de ciclos para 2MS.</ul>

## Dudas

## Bugs

## Lista de cambios
