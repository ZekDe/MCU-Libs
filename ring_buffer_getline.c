#include "ring_buffer_getline.h"

#include "SW2023.h"

static ring_buffer_t rx_ring = {0};

char getline_buffer[GETLINE_BUFFER];

void ringBufferPut(char c)
{
    int next_head = (rx_ring.head + 1) % RING_BUFFER_SIZE;
    
    if(next_head != rx_ring.tail)
    {
        rx_ring.buffer[rx_ring.head] = c;
        rx_ring.head = next_head;
    }
}

int ringBufferGet(char *c)
{
    if(rx_ring.head == rx_ring.tail)
        return 0;
    
    *c = rx_ring.buffer[rx_ring.tail];
    rx_ring.tail = (rx_ring.tail + 1) % RING_BUFFER_SIZE;
    return 1;
}

// UART interrupt handler
void getline_isr(void)
{    
    char c = UART_READ(UART0);
    ringBufferPut(c);
}

// Main loop'ta çagrilacak
int getline(void)
{
    static int idx = 0;
    static char prev_c = 0;
    char c;
    
    while(ringBufferGet(&c))
    {
        if (c == '\n' || c == '\r')
        {
            // Ardisik newline karakterlerini atla (\r\n veya \n\r)
            if (prev_c == '\n' || prev_c == '\r')
            {
                prev_c = c;
                continue;
            }
            
            // Geçerli satir varsa döndür
            if(idx > 0)
            {
                getline_buffer[idx] = '\0';
                int len = idx;
                idx = 0;
                prev_c = c;
                return len;
            }
        }
        else if (c >= 32 && c <= 126)  // Yazdirilabilir karakterler
        {
            if(idx < GETLINE_BUFFER - 1)
            {
                getline_buffer[idx++] = c;
            }
            else
            {
                // Buffer doldu - satiri sifirla
                idx = 0;
            }
        }
        
        prev_c = c;
    }
    
    return 0;  // Henüz tam satir yok
}
