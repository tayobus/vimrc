typedef struct button {
    int pin;
    int pout;
    int polling_rate;
    void *(*onLongClick) ();
    void *(*onPressDown) ();
    void *(*onPressUp) ();
} BUTTON;

pthread_t* initButton(BUTTON button);