#include <iostream>
#include <playerHelper.c>
#include <vector>
#include <time.h>
#include <math.h>

void printBoard(struct State *state);
void safeCopy(char *dest, char *src, int destSize, int numbytes)
{
    memset(dest, 0, destSize);
    memcpy(dest, src, numbytes);
}

class Node
{
public:
    State state;
    std::vector<Node> children;
    int visited = 0;
    int wins = 0;
    int depth = 0;
    struct Node *parent;

    // Returns if this node is a node.
    static bool isTerminal(Node *node)
    {
        return node->state.numLegalMoves == 0;
    }

    // Returns if this node is a leaf node.
    // leaf node means the node can be expanded further.
    static bool isLeaf(Node *node)
    {
        return node->children.size() < node->state.numLegalMoves;
    }

    // Expands this node.
    // During expansion, all children are produced for the node,
    // and playouts happen.
    static void expand(Node *node)
    {
        fprintf(stderr, "expand\n");
        for (int i = 0; i < node->state.numLegalMoves; i++)
        {
            // fprintf(stderr, "expand %d\n", i);
            State newState;
            memcpy(&newState, &node->state, sizeof(State));
            performMove(&newState, i);

            Node newNode = Node(newState, node);
            int playout_score = Node::run_playout(&newState, 50) == 1 ? 1 : 0;

            node->children.push_back(newNode);
            backprop(&newNode, playout_score, 1);
        }

        fprintf(stderr, "Done expanding.\n");
        // fprintf(stderr, "Node children count: %d\n", node->children.size());
    }

    // Select.
    static void select(Node *node)
    {
        fprintf(stderr, "select\n");
        if (Node::isTerminal(node))
        {
            // fprintf(stderr, "terminal\n");
            node->visited += 1;
            backprop(node, 0, 1);
        }

        if (Node::isLeaf(node))
        {
            // fprintf(stderr, "leaf\n");
            // Expand further.
            Node::expand(node);
            return;
        }

        if (node->children.size() == 0)
        {
            // TODO: Fix this.
            fprintf(stderr, "Children size 0! panic!\n");
            return;
        }

        fprintf(stderr, "check again.\n");
        double bestutility = utility(&node->children[0], node);
        Node *selected = &node->children[0];

        fprintf(stderr, "chek again before loop.");
        for (int i = 0; i < node->children.size(); i++)
        {
            Node child = node->children[i];
            // fprintf(stderr, "check again loop.");
            double util = utility(&child, node);
            if (util > bestutility)
            {
                bestutility = util;
                selected = &child;
            }
        }
        // fprintf(stderr, "check here.");
        Node::select(selected);
    }

    // Playout
    static int run_playout(State *state, int maxmoves)
    {
        if (maxmoves-- == 0)
        {
            return 0;
        }
        if (state->numLegalMoves == 0)
        {
            fprintf(stderr, "No more movex. lost\n'):\n");
            return -1;
        }

        int index = rand() % state->numLegalMoves;
        State newState;
        mempcpy(&newState, state, sizeof(State));
        performMove(&newState, index);

        return -Node::run_playout(&newState, maxmoves);
    }

    // Utility.
    static double utility(Node *node, Node *parent)
    {
        // printBoard(&node->state);
        double exploit = ((double)node->wins / (double)node->visited);
        double explore = 1.4 * sqrt(log(parent->visited) / node->visited);

        fprintf(stderr, "Parent and child : %d/%d and %d/%d\n", parent->wins, parent->visited, node->wins, node->visited);
        return exploit + explore;
    }

    // Backprop.
    static void backprop(Node *node, int wins, int gameplays)
    {
        // fprintf(stderr, "backprop\n");
        while (node != nullptr)
        {
            node->wins += wins;
            node->visited += gameplays;

            wins = gameplays - wins;
            node = node->parent;
        }
    }

    Node(State state, Node *parent)
    {
        state = state;
        parent = parent;
        if (parent != nullptr)
        {
            depth = parent->depth + 1;
        }
    }
};

bool stop(struct timespec *clock)
{
    struct timespec current_time;
    clock_gettime(CLOCK_REALTIME, &current_time);

    // Calculate the elapsed time in seconds
    long elapsedSeconds = current_time.tv_sec - clock->tv_sec;

    // Check if the elapsed time exceeds the time limit
    if (elapsedSeconds >= SecPerMove - 1)
    {
        return true; // Stop the operation
    }
    else
    {
        return false; // Continue the operation
    }
}

void FindBestMove(int player, char board[8][8], char *bestmove)
{
    int bestMoveIndex;
    struct State state;

    setupBoardState(&state, player, board);

    // Start the timer.
    struct timespec timer;
    clock_gettime(CLOCK_REALTIME, &timer);

    // generate the node, and start selection.
    Node rootnode = Node(state, nullptr);
    while (!stop(&timer))
        Node::select(&rootnode);

    for (auto child : rootnode.children)
    {
        fprintf(stderr, "Child playout: (%d/%d)\n", child.wins, child.visited);
    }

    // Calculate the child path to take.
    // But first, check root utility.
    fprintf(stderr, "Utilitz: %d/%d.", rootnode.wins, rootnode.visited);

    int selindex = 0;
    fprintf(stderr, "child count for root node: %d\n", rootnode.children.size());
    double selscore = rootnode.children[0].visited;

    for (int i = 0; i < rootnode.children.size(); i++)
    {
        Node child = rootnode.children[i];
        double score = rootnode.children[i].visited;
        fprintf(stderr, "Got score %d/%d for branch %d", child.wins, child.visited, i);
        if (score > selscore)
        {
            selscore = score;
            selindex = i;
        }
    }

    bestMoveIndex = selindex;

    safeCopy(bestmove, state.movelist[bestMoveIndex], MaxMoveLength, MoveLength(state.movelist[bestMoveIndex]));
}

void printBoard(struct State *state)
{
    int y, x;

    for (y = 0; y < 8; y++)
    {
        for (x = 0; x < 8; x++)
        {
            if (x % 2 != y % 2)
            {
                if (empty(state->board[y][x]))
                {
                    fprintf(stderr, " ");
                }
                else if (king(state->board[y][x]))
                {
                    if (color(state->board[y][x]) == 2)
                        fprintf(stderr, "B");
                    else
                        fprintf(stderr, "A");
                }
                else if (piece(state->board[y][x]))
                {
                    if (color(state->board[y][x]) == 2)
                        fprintf(stderr, "b");
                    else
                        fprintf(stderr, "a");
                }
            }
            else
            {
                fprintf(stderr, " ");
            }
        }
        fprintf(stderr, "\n");
    }
}
