/*****************************************
Brandon Baker, Brendan Coffman
Dr. Bobeldyk
Operating Systems Concepts
Project 2 bakeOff.c

This program simulates bakers in a shared
kitchen with limited access to resources at
a time. Threads are used to represent bakers
while semaphores are used to limit access to
each resource.
******************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <sys/sem.h>
#include <stdbool.h>
#include <pthread.h>

#define createItem(name, quantity)                                                  \
    if ((name = semget(IPC_PRIVATE, 1, IPC_CREAT | S_IRUSR | S_IWUSR)) == -1) {     \
        perror("Failed to create semaphore\n");                                     \
        exit(1);                                                                    \
    }                                                                               \
                                                                                    \
    if (semctl(name, 0, SETVAL, quantity) == -1) {                                  \
        perror("Failed to initialize semaphore with value of 1\n");                 \
        exit(1);                                                                    \
    }

#define endItem(name, quantity)                     \
    if (semctl(name, 0, IPC_RMID) == -1) {          \
        perror("Semaphore deallocation failed\n");  \
        exit(1);                                    \
    }

#define intList(name, quantity)      \
    int name;                       

//A list of all the limited resources of the project that can have the passed macro occur to them (which behaves like a passed function) happen to them
//allows for things like resourceList(createItem) to be repeated for every single resource and do everything in the createItem for each resource
#define resourceList(OP)             \
    OP(Mixer, 2)                     \
    OP(Pantry, 1)                    \
    OP(Refrigerator, 2)              \
    OP(Bowl, 3)                      \
    OP(Spoon, 5)                     \
    OP(Oven, 1)

//Enum of items that are in the pantry
typedef enum pantryItems
{
    flour = 0,
    sugar,
    yeast,
    bakingSoda,
    salt,
    cinnamon
} pantryItems_enum;

//Enum of items that are in the fridge
typedef enum fridgeItems
{
    eggs,
    milk,
    butter
} fridgeItems_enum;

//Global variables
resourceList(intList)
struct sembuf waitOperation = {0, -1, SEM_UNDO}; // Wait operation
struct sembuf signalOperation = {0, 1, SEM_UNDO}; // Signal operation
bool ramsied = false;
int bakerToGetRamsied = 1;
int stepToGetRamsiedBy = 35;

//Function to update the color of text for the baker
void updateColor(int bakerNum)
{
    switch(bakerNum) {
        case 0:
            printf("\033[0;31m"); // Red
            break;
        case 1:
            printf("\033[0;32m"); // Green
            break;
        case 2:
            printf("\033[0;33m"); // Yellow
            break;
        case 3:
            printf("\033[0;34m"); // Blue
            break;
        case 4:
            printf("\033[0;35m"); // Magenta
            break;
        case 5:
            printf("\033[0;36m"); // Cyan
            break;
        default:
            break;
    }
}

//Convert the enum for the fridge items back to a string
char* fridgeEnumToString(fridgeItems_enum item)
{
    switch(item) {
        case eggs:
            return "eggs";
            break;
        case milk:
            return "milk";
            break;
        case butter:
            return "butter";
            break;
        default:
            return "error";
            break;
    }
}

//Convert the pantry item enum back to a string
char* pantryEnumToString(pantryItems_enum item)
{
    switch(item) {
        case flour:
            return "flour";
            break;
        case sugar:
            return "sugar";
            break;
        case yeast:
            return "yeast";
            break;
        case bakingSoda:
            return "baking soda";
            break;
        case salt:
            return "salt";
            break;
        case cinnamon:
            return "cinnamon";
            break;
        default:
            break;
    }
}

//Check if the user got ramsied based on the step and if the baker is the one to get ramsied at a certain step
bool checkIfRamsied(int bakerNum)
{
    bool bakerRamsied = false;
    if(bakerNum == bakerToGetRamsied && !ramsied)
    {
        stepToGetRamsiedBy -= 1;
        bakerRamsied = stepToGetRamsiedBy == 0;
        ramsied = bakerRamsied;
    }
    return bakerRamsied;
}


bool getIngredientFromRefrigerator(fridgeItems_enum ingredient, int bakerNum)
{
    if (semop(Refrigerator, &waitOperation, 1) == -1) {
        perror("Failed to wait on semaphore\n");
        return false;
    }
    int fridgeNum = semctl(Refrigerator, 0, GETVAL);
    updateColor(bakerNum);
    printf("Baker %d entered Refrigerator %d. Grabbed %s.\n", bakerNum, fridgeNum, fridgeEnumToString(ingredient));
    

    if (semop(Refrigerator, &signalOperation, 1) == -1) {
        perror("Failed to signal semaphore\n");
        return false;
    }

    updateColor(bakerNum);
    printf("Baker %d is done using the Refrigerator %d\n", bakerNum, fridgeNum);

    return !checkIfRamsied(bakerNum);
}

bool getIngredientFromPantry(pantryItems_enum ingredient, int bakerNum)
{
    if (semop(Pantry, &waitOperation, 1) == -1) {
        perror("Failed to wait on semaphore\n");
        return false;
    }
    updateColor(bakerNum);
    printf("Baker %d entered the pantry. Grabbed %s.\n", bakerNum, pantryEnumToString(ingredient));
    

    if (semop(Pantry, &signalOperation, 1) == -1) {
        perror("Failed to signal semaphore\n");
        return false;
    }

    updateColor(bakerNum);
    printf("Baker %d is done using the Pantry\n", bakerNum);

    return !checkIfRamsied(bakerNum);
}

//Grab the mixer, bowl, and spoon to mix ingredients before baking
bool mixIngredients(int bakerNum)
{
    if (semop(Mixer, &waitOperation, 1) == -1) {
        perror("Failed to wait on semaphore\n");
        return false;
    }
    updateColor(bakerNum);
    printf("Baker %d using the Mixer\n", bakerNum);
    
    if (semop(Bowl, &waitOperation, 1) == -1) {
        perror("Failed to wait on semaphore\n");
        return false;
    }
    updateColor(bakerNum);
    printf("Baker %d using the Bowl\n", bakerNum);
    
    if (semop(Spoon, &waitOperation, 1) == -1) {
        perror("Failed to wait on semaphore\n");
        return false;
    }
    updateColor(bakerNum);
    printf("Baker %d using the Spoon\n", bakerNum);
    
    updateColor(bakerNum);
    printf("Baker %d mixing the ingredients\n", bakerNum);
    
    if (semop(Mixer, &signalOperation, 1) == -1) {
        perror("Failed to signal semaphore\n");
        return false;
    }
    updateColor(bakerNum);
    printf("Baker %d is done using the Mixer\n", bakerNum);
    if (semop(Bowl, &signalOperation, 1) == -1) {
        perror("Failed to signal semaphore\n");
        return false;
    }
    updateColor(bakerNum);
    printf("Baker %d is done using the Bowl\n", bakerNum);
    if (semop(Spoon, &signalOperation, 1) == -1) {
        perror("Failed to signal semaphore\n");
        return false;
    }
    updateColor(bakerNum);
    printf("Baker %d is done using the Spoon\n", bakerNum);

    return !checkIfRamsied(bakerNum);
}

//Use the oven. Also contains a sleep because ovens take time to cook
bool useOven(int bakerNum)
{
    if (semop(Oven, &waitOperation, 1) == -1) {
        perror("Failed to wait on semaphore\n");
        return false;
    }
    updateColor(bakerNum);
    printf("Baker %d using the oven\n", bakerNum);
    sleep(1);

    if (semop(Oven, &signalOperation, 1) == -1) {
        perror("Failed to signal semaphore\n");
        return false;
    }
    updateColor(bakerNum);
    printf("Baker %d is done using the Oven\n", bakerNum);

    return true;
}

//Top level bake cookie
bool bakeCookie(int bakerNum)
{
    bool successful = true;
    successful = successful && getIngredientFromPantry(flour, bakerNum);
    successful = successful && getIngredientFromPantry(sugar, bakerNum);
    successful = successful && getIngredientFromRefrigerator(milk, bakerNum);
    successful = successful && getIngredientFromRefrigerator(butter, bakerNum);
    successful = successful && mixIngredients(bakerNum);
    successful = successful && useOven(bakerNum);

    if(!successful)
    {
        updateColor(bakerNum);
        printf("Baker %d got ramsied! Trying again...\n", bakerNum);
        return false;
    }
    else
    {
        updateColor(bakerNum);
        printf("Baker %d is done making cookies\n", bakerNum);
    }
    return successful;
}

//Top level bake pancake
bool bakePancake(int bakerNum)
{
    bool successful = true;
    successful = successful && getIngredientFromPantry(flour, bakerNum);
    successful = successful && getIngredientFromPantry(sugar, bakerNum);
    successful = successful && getIngredientFromPantry(bakingSoda, bakerNum);
    successful = successful && getIngredientFromPantry(salt, bakerNum);
    successful = successful && getIngredientFromRefrigerator(milk, bakerNum);
    successful = successful && getIngredientFromRefrigerator(butter, bakerNum);
    successful = successful && mixIngredients(bakerNum);
    successful = successful && useOven(bakerNum);

    if(!successful)
    {
        updateColor(bakerNum);
        printf("Baker %d got ramsied. Trying again...\n", bakerNum);
        return false;
    }
    else
    {
        updateColor(bakerNum);
        printf("Baker %d is done making pancakes\n", bakerNum);
    }
    return successful;
}

//Top level bake Pizza dough
bool bakePizzaDough(int bakerNum)
{
    bool successful = true;
    successful = successful && getIngredientFromPantry(yeast, bakerNum);
    successful = successful && getIngredientFromPantry(sugar, bakerNum);
    successful = successful && getIngredientFromPantry(salt, bakerNum);
    successful = successful && mixIngredients(bakerNum);
    successful = successful && useOven(bakerNum);

    if(!successful)
    {
        updateColor(bakerNum);
        printf("Baker %d got ramsied! Trying again...\n", bakerNum);
        return false;
    }
    else
    {
        updateColor(bakerNum);
        printf("Baker %d is done making pizza dough\n", bakerNum);
    }return successful;
}

//Top level bake soft pretzels
bool bakeSoftPretzels(int bakerNum)
{
    bool successful = true;
    successful = successful && getIngredientFromPantry(flour, bakerNum);
    successful = successful && getIngredientFromPantry(sugar, bakerNum);
    successful = successful && getIngredientFromPantry(salt, bakerNum);
    successful = successful && getIngredientFromPantry(bakingSoda, bakerNum);
    successful = successful && getIngredientFromRefrigerator(eggs, bakerNum);
    successful = successful && mixIngredients(bakerNum);
    successful = successful && useOven(bakerNum);

    if(!successful)
    {
        updateColor(bakerNum);
        printf("Baker %d got ramsied! Trying again...\n", bakerNum);
    }
    else
    {
        updateColor(bakerNum);
        printf("Baker %d is done making pretzels\n", bakerNum);
    }
    return successful;
}

//Top level bake cinnamon rolls
bool bakeCinnamonRolls(int bakerNum)
{
    bool successful = true;

    successful = successful && getIngredientFromPantry(flour, bakerNum);
    successful = successful && getIngredientFromPantry(sugar, bakerNum);
    successful = successful && getIngredientFromPantry(salt, bakerNum);
    successful = successful && getIngredientFromRefrigerator(butter, bakerNum);
    successful = successful && getIngredientFromRefrigerator(eggs, bakerNum);
    successful = successful && getIngredientFromPantry(cinnamon, bakerNum);
    successful = successful && mixIngredients(bakerNum);
    successful = successful && useOven(bakerNum);

    if(!successful)
    {
        updateColor(bakerNum);
        printf("Baker %d got ramsied! Starting Over...\n\n\n", bakerNum);

    }
    else
    {
        updateColor(bakerNum);
        printf("Baker %d is done making cinnamon rolls\n", bakerNum);
    }
    return successful;
}

//What each baker is doing. The bake num is passed in
void *baker_thread(void *arg) {
    int bakerNum = *((int *)arg);

    if(!bakeCookie(bakerNum))
    {
        ramsied = true;
        bakeCookie(bakerNum);
    }

    if(!bakePancake(bakerNum))
    {
        ramsied = true;
        bakePancake(bakerNum);
    }

    if(!bakePizzaDough(bakerNum))
    {
        ramsied = true;
        bakePizzaDough(bakerNum);
    }

    if(!bakeSoftPretzels(bakerNum))
    {
        ramsied = true;
        bakeSoftPretzels(bakerNum);
    }

    if(!bakeCinnamonRolls(bakerNum))
    {
        ramsied = true;
        bakeCinnamonRolls(bakerNum);
    }
    updateColor(bakerNum);
    printf("Baker %d finished!\n", bakerNum);
    return NULL;
}

int main(void)
{
    //Uses the resource list macro that contains all the limited resources and expands it out with the create item
    //which creates semaphores for each item in the list
    resourceList(createItem);

    pthread_t bakers[3];

    // Create threads
    for (int i = 0; i < 3; i++) {
        int *bakerNum = malloc(sizeof(int));
        *bakerNum = i + 1;
        pthread_create(&bakers[i], NULL, baker_thread, (void *)bakerNum);
    }

    // Wait for thread to finish
    for(int i = 0; i < 3; i++){
        pthread_join(bakers[i], NULL);
    }
    
    //Preforms the endItem macro on each of the items in the resource list
    //deletes all of the semaphores created at the beginning of main()
    resourceList(endItem);
    return 0;
}