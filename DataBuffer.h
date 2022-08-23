#ifndef _DataBuffer_h
#define _DataBuffer_h 1

#define DATABUFF_MAX_DATA   300

class DataBuffer {
    public:
        DataBuffer(void);
        void clear(void);
        int read(uint8_t *data);
        int readLast(void);
        int get(int idx);
        int getLength();
        int write(uint8_t data);
        int setComplete();
        int isComplete();
        
    private:
        volatile int _readPos;
        volatile int _len;
        volatile uint8_t _buff[DATABUFF_MAX_DATA];
        volatile uint8_t _complete;    
};

/*
typedef struct dataBuffer {
    volatile uint8_t readPos;
    volatile uint8_t len;
    volatile uint8_t buff[DATABUFF_MAX_DATA];
    volatile uint8_t complete;
} dataBuffer_t;


extern void dataBuffClear(dataBuffer_t *buff);
extern int dataBuffRead(dataBuffer_t *buff, uint8_t *data);
extern int dataBuffWrite(dataBuffer_t *buff, uint8_t data);
extern int dataBuffSetComplete(dataBuffer_t *buff);
extern int dataBuffIsComplete(dataBuffer_t *buff);

#define dataBuffInit dataBuffClear
*/

#endif
