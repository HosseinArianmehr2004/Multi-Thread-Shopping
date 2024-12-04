#include <stdio.h>

int main() {
    // 2D array of characters
    char fruits[5][20] = {
        "Apple",
        "Banana",
        "Cherry",
        "Date",
        "Elderberry"
    };

    // Calculate the number of strings
    int numFruits = sizeof(fruits) / sizeof(fruits[0]);

    // Print each string
    for (int i = 0; i < numFruits; i++) {
        printf("%s\n", fruits[i]);
    }

    return 0;
}
