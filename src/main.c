/**
  ******************************************************************************
  * @file    main.c
  * @author  Deepak Ravikumar
  * @version V1.0
  * @date    22-Apr-2019
  * @brief   Default main function, initializes everything
  ******************************************************************************
*/

#include "graph.h"

int main(void)
{
    // Init();
    // Os Init()??;

    // Negotiate RfAddr
    // Build graph
    buildDummyGraph();
    colorGraph();

    // Color graph()
    // Wait for trigger (antoher device or motion sensor)

    for(;;)
        asm ("wfi");
}
