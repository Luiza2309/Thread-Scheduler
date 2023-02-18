so_init
    - am facut verificarile si am alocat componentele schedulerului

so_fork
    - am facut verificarile
    - am alocat si initializat un thread
    - am creat threadul cu pthread_create
    - nu puteam sa dau direct functia func din cauza argumentelor care sunt int si nu void
    - am facut o functie auxiliara start_thread care imi pune threadul pe wait
    - cand threadul va fi pus pe running, acesta va executa functia handler din start_thread
    - daca este primul thread creat este pus direct in running
    - altfel, se verifica daca prioritatea threadului nou creat e mai mare decat cea a celui de pe running si se face switch
    - altfel, se adauga threadul nou creat in ready si se scade din timpul threadului running
    - se verifica daca threadului running si-a terminat cuanta si se face switch cu primul thread din ready daca au aceeasi prioritate
    - altfel, se reactualizeaza cuanta threadului running

so_exec
    - am facut verificarile cuantei la fel ca la so_fork

so_end
    - am dezalocat tot