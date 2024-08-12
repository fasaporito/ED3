/* Primer tarea de la materia Electrónica Digital 3, cohorte 2024.
  Autor: Franco Saporito
  Descripción: El programa genera dos números naturales aletorios del 1 al 20 y pide realizar una operación (suma, resta o multiplicación).
  El usuario debe realizar la operación mentalmente y escribir el resultado. Luego el programa depende si el resultado ingresado es correcto o incorrecto.
*/

#include <stdio.h>
#include <stdlib.h>

int main() {
    int num1, num2, res, resp;
    char op;

    printf("Bienvenido al juego para hacer cálculos mentales!\n\n");

    srand(1);

    // Generar dos números aleatorios entre 1 y 20
    num1 = rand() % 20 + 1;
    num2 = rand() % 20 + 1;

    // Para seleccionar aleatoriamente una operación se usa la función rand
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
    printf("Realizá la siguiente cuenta: ");
    printf("%d %c %d = ", num1, op, num2);
    scanf("%d", &resp);

    // Verificar si el resultado es correcto
    if (resp == res) {
        printf("Le pegaste!\n");
    } else {
        printf("Seguí participando. El resultado correcto es %d.\n", res);
    }

    return 0;
}