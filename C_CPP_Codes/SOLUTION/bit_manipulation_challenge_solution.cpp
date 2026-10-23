#include <stdio.h>
#include <stdint.h>

void print_binary(uint32_t n, int shift){
    // multiple by 8 because uint32_t is BYTE type, so sizeof() returns 4 as 32 bits = 4 bytes
    for (int i = sizeof(n)*8 - 1; i >= 0; i--){
        int bit = ((n << shift) >> i) & 1;
        printf("%d", bit);
    }
    printf("\n");
    //printf("Size of n: %d\n", (int)(sizeof(n)*8));
}
uint32_t combine(uint32_t x, uint32_t y){
    uint32_t comb = x | (y << 8);
    return comb; 
}

uint32_t get_x(uint32_t comb){
    return comb & 0X000000FF;
}

uint32_t get_y(uint32_t comb){
    return comb & 0X0000FF00;
}

int main(){
    uint32_t x = 21;
    uint32_t y = 17;

    uint32_t x_mask = 0x000000FF;
    uint32_t y_mask = 0x0000FF00;

    uint32_t comb = combine(x, y);
    printf("x = ");
    print_binary(x, 0);
    printf("y = ");
    print_binary(y, 0);
    printf("comb&x_mask: ");
    print_binary(comb & x_mask, 0);
    printf("comb&y_mask: ");
    print_binary(comb & y_mask, 0);
    printf("The combination of x and y: ");
    print_binary(comb, 0);
    printf("The recovered x: ");
    print_binary(get_x(comb), 0);
    printf("The recovered y: ");
    print_binary(get_y(comb), 0);

}