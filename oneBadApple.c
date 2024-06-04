#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/wait.h>

#define READ 0
#define WRITE 1

#define CREATE_PIPE_WITH_ERROR_CHECK(pipeName) \
    if (pipe(pipeName) == -1)                  \
    {                                          \
        perror("Failed pipe creation\n");      \
        exit(1);                               \
    }

typedef struct {
    int header;
    char message[100];
    int lastHeldBy;
} apple_t;

typedef struct {
    int forward[2];
    int receive[2];
    int id;
} node_t;

int nodeCount;
node_t node;

void sigHandler();
void userSigHandler();
void createChildrenNodes(int numNodes, node_t *parentNode, node_t *rootNode);
void processApple();


int main()
{
    signal(SIGINT, sigHandler);
    int numNodes;
    int toNode;
    apple_t apple;

    printf("How many nodes would you like to create? ");
    fflush(stdout);
    scanf("%d", &nodeCount);
    getchar();

    node_t root;
    root.id = 0;
    CREATE_PIPE_WITH_ERROR_CHECK(root.forward);
    CREATE_PIPE_WITH_ERROR_CHECK(root.receive);
    printf("Created node %d\n", root.id);

    createChildrenNodes(nodeCount, &root, &root);
    memcpy(&node, &root, sizeof(node));
    while (1)
    {
        printf("Which node would you like to write to? ");
        fflush(stdout);
        scanf("%d", &toNode);

        printf("Enter a message: ");
        fflush(stdout);
        scanf(" %[^\n]", apple.message); // Read until newline character

        // Send the apple to the specified node
        apple.header = toNode;
        apple.lastHeldBy = 0;
        printf("Node %d has received the apple from User\n", node.id);
        printf("Node %d has sent the Apple to Node: %d\n", node.id, (node.id+1) % nodeCount);
        write(root.forward[WRITE], &apple, sizeof(apple_t));

        // Receive the apple back
        read(root.receive[READ], &apple, sizeof(apple_t));
        printf("Node 0 received the apple back. Apple message: %s\n", 
                        strlen(apple.message) > 0 ? apple.message : "empty");
    }
}

void processApple(void)
{
    // Listen for the apple and check whether it belongs
    pid_t pid = fork();
    if(pid == 0) //child
    {
        while(1)
        {
            apple_t apple;
            int bytesRead = read(node.receive[READ], &apple, sizeof(apple_t));

            printf("Node %d has received the apple from %d\n", node.id, apple.lastHeldBy);
            if(apple.header == node.id)
            {
                printf("Apple belongs to %d reading: %s\n", node.id, apple.message);

                // Clears the message out
                memset(apple.message, 0, sizeof(apple.message));
                printf("Apple message has been cleared\n");
            }

            // Pass apple to next node
            printf("Node %d has sent the Apple to Node: %d\n", node.id, (node.id+1) % nodeCount);
            apple.lastHeldBy = node.id;
            write(node.forward[WRITE], &apple, sizeof(apple_t)); 
        }
    }
}

void createChildrenNodes(int numNodes, node_t *parentNode, node_t *rootNode)
{

    node_t newNode;
    newNode.id = numNodes-1;
    if(newNode.id < 1){
        return;
    }

    if(newNode.id == 1)
    {
        memcpy(newNode.receive, rootNode->forward, sizeof(newNode.receive));
        memcpy(newNode.forward, parentNode->receive, sizeof(newNode.forward));
        memcpy(&node, &newNode, sizeof(node));
        processApple();
    }
    else
    {
        memcpy(newNode.forward, parentNode->receive, sizeof(newNode.forward));
        CREATE_PIPE_WITH_ERROR_CHECK(newNode.receive);
        createChildrenNodes(numNodes - 1, &newNode, rootNode);
        memcpy(&node, &newNode, sizeof(node));
        processApple();
    }
    printf("Created node %d\n", newNode.id);
}

void sigHandler()
{
    printf("Node %d shutting down\n", node.id);
    exit(0);    
}