/* Primer tarea de la materia Electrónica Digital 3, cohorte 2024.
  Autor: Franco Saporito
  Descripción: El programa genera dos números naturales aletorios del 1 al 20 y pide realizar una operación (suma, resta o multiplicación).
  El usuario debe realizar la operación mentalmente y escribir el resultado. Luego el programa depende si el resultado ingresado es correcto o incorrecto.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>  // Para usar strcmp
#include <time.h>   //Usamos esta librería para generar numeros aleatorios tomando la hora como punto de partida en la función srand.

int main() {
    int num1, num2, res, resp;
    char op;
    char entrada[10];  // Para almacenar lo escrito por el usuario

    printf("Bienvenido al juego para hacer cálculos mentales!\n");
    printf("Cuando no quieras jugar más escribí 'salir'\n\n");

    srand(time(NULL));

    while (1) {
        // Genera dos números aleatorios entre 1 y 20
        num1 = rand() % 20 + 1;
        num2 = rand() % 20 + 1;

        // Selecciona aleatoriamente una operación
        int opAl = rand() % 3;
        switch(opAl) {
            case 0:
                op = '+';
                res = num1 + num2;
                break;
            case 1:
                op = '-';
                res = num1 - num2;
                break;
            case 2:
                op = '*';
                res = num1 * num2;
                break;
        }

        // Pide al usuario que realice la operación
        printf("Realizá la siguiente cuenta: ");
        printf("%d %c %d = ", num1, op, num2);
        scanf("%s", entrada);  // Lee la entrada del usuario como una cadena

        // Verifica si el usuario desea salir
        if (strcmp(entrada, "salir") == 0) {
            printf("Gracias por jugar. ¡Hasta la próxima!\n");
            break;
        }

        // Convierte la entrada a un número para verificar la respuesta
        resp = atoi(entrada);

        // Verifica si el resultado es correcto
        if (resp == res) {
            printf("¡Le pegaste!\n");
        } else {
            printf("Seguí participando. El resultado correcto es %d.\n", res);
        }
        printf("\n");
    }

    return 0;
}
