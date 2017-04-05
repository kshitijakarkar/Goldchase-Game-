// Kshitija Karkar
// GoldChase program with signal handling


#include "goldchase.h"
#include "Map.h"
#include "fancyRW.h"
#include <sys/ipc.h>
#include <sys/sem.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <cstring>
#include <algorithm>
#include <cctype>

#include <cstdio>
#include <cstdlib>
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <semaphore.h>
#include <sys/mman.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>            /* time */
#include <signal.h>
#include <mqueue.h>
#include <unistd.h>
#include <sys/types.h>
#include <cassert>

char debugStr[100];
int debugFD=-1;
//The macro for writing debugging messages to a named pipe
#define DEBUG(msg, ...) do{sprintf(debugStr,msg, ##__VA_ARGS__); WRITE(debugFD,debugStr, strlen(debugStr));}while(0)
//The macro that contains the full pathname to the named pipe
//#define NAMEDPIPE "/home/tgibson/studentwork/kshitija/err_out"
#define NAMEDPIPE "/home/kshitija/CSCI611/daemon/mypipe"

using namespace std;

struct GameBoard {
  int rows; //4 bytes
  int cols; //4 bytes
  unsigned char players;
  pid_t pid[5];
  int daemonID;
  unsigned char map[0];
};


Map *ptr;
void handle_interrupt(int);
void clean_up(int);
void read_message(int);
void read_message1(int);
void readQueue(string);
void writeQueue(string,char *);
void client_daemon(const char *);
mqd_t readqueue_fd;
mqd_t sock_readqueue[5];
string msg_q[5];
string sock_q[5];
int fd;
unsigned char c;
void create_daemon();
sem_t *first_semaphore;
//unsigned char* local_mapcopy;
bool check_signal = false;
GameBoard* goldmap;
//GameBoard* gm;
unsigned char *local_copy;

void handle_interrupt_usr(int);
void sig_up(int);
//void read_message_from_socket(int);
unsigned char sock_byte;

int pipefd[2];
int sockfd; //file descriptor for the socket
int status; //for error checking

int new_sockfd;
//int sock_fd;
//GameBoard *gbp;

void continous_while();

short size;
unsigned char temp3;

int main(int argc,char *argv[])
{
  debugFD=open(NAMEDPIPE,O_WRONLY);
  assert(debugFD!=-1); //go no further if we couldn't open the named pipe
  int value=0;
  int trunc;
  int fool=0;
  int plyr=0;
  int check=0;
  int calc_row =0;
  int calc_col=0;
  int mul=0;

  int cnt = 0;
  int count=0;

  bool found=false;    //set value to true if real gold found
  bool set = false;

  goldmap=0;

  unsigned char current_player=0;

  mqd_t writequeue_fd;

  struct sigaction action_jackson;
  action_jackson.sa_handler=handle_interrupt;
  sigemptyset(&action_jackson.sa_mask);
  action_jackson.sa_flags=0;
  action_jackson.sa_restorer=NULL;

  sigaction(SIGUSR1, &action_jackson, NULL);

  struct sigaction exit_handler;     //send SIGINT to cleanup the message queue.
  exit_handler.sa_handler=clean_up;
  sigemptyset(&exit_handler.sa_mask);
  exit_handler.sa_flags=0;
  sigaction(SIGINT, &exit_handler, NULL);
  sigaction(SIGHUP, &exit_handler, NULL);
  sigaction(SIGTERM, &exit_handler, NULL);

  struct sigaction action_to_take;           //signal to read message
  action_to_take.sa_handler=read_message;
  sigemptyset(&action_to_take.sa_mask);
  action_to_take.sa_flags=0;
  sigaction(SIGUSR2, &action_to_take, NULL);

  msg_q[0]="/game_player1_mq";
  msg_q[1]="/game_player2_mq";
  msg_q[2]="/game_player3_mq";
  msg_q[3]="/game_player4_mq";
  msg_q[4]="/game_player5_mq";

  /*   for(int i=0;i<5;i++)
       {
       sock_readqueue[5] = -1;
       }*/
  if(argc == 2)
  {
    first_semaphore=sem_open("/multiplayergames",O_CREAT|O_EXCL|O_RDWR,S_IROTH| S_IWOTH|S_IRGRP| S_IWGRP| S_IRUSR| S_IWUSR,1);

    if(first_semaphore != SEM_FAILED)
    {
      const char *s = argv[1];
      client_daemon(s);
    }
  }



  if(msg_q[0]!="/game_player1_mq")
  {
    cerr << "Message Queue" << endl;
    exit(1);
  }


  first_semaphore=sem_open("/multiplayergames",O_CREAT|O_EXCL|O_RDWR,S_IROTH| S_IWOTH|S_IRGRP| S_IWGRP| S_IRUSR| S_IWUSR,1);

  if(first_semaphore==SEM_FAILED)
  {
    if(errno!=EEXIST)
    {
      perror("Opening Semaphore :");
      exit(1);
    }
    DEBUG("semaphore failed to open because it already exists. This is OK\n");
  }


  if(first_semaphore != SEM_FAILED) //first player
  {
    cout<<"sem not failed ";
    if(sem_wait(first_semaphore)==-1) //sem wait
    {
      perror("Wait value :");
    }
    fd=shm_open("/shared_map", O_RDWR|O_CREAT|O_EXCL, S_IRUSR|S_IWUSR);
    if(fd == -1)
    {
      perror("Opening shared memory ");
    }

    string line,k;
    int no_of_col = 0;
    int no_of_row = 0;
    int number_of_gold=0;
    int count = 0;
    vector <string> myvector;

    ifstream myfile("mymap.txt");

    if(myfile.is_open())             //read file
    {
      while(getline(myfile,line))
      {
        count++;
        if(count == 1)
        {
          number_of_gold = stoi(line);

        }
        else
        {
          if(count == 2)
          {
            no_of_col=line.size();
          }

          myvector.push_back(line);

        }
      }           //end of while

    } //end of if
    else
    {
      cout<<"Error opening file "<<endl;
      return 0;
    }

    no_of_row=count-1;
    mul= no_of_row*no_of_col;

    string m;
    vector <string> ::iterator it;          //store in vector

    for(vector<string>::const_iterator i = myvector.begin(); i != myvector.end(); ++i)
      m+=*i;


    myfile.close();

    trunc=ftruncate(fd,(no_of_col*no_of_row)+sizeof(GameBoard));

    if(trunc==-1)
    {
      perror("File truncate :");
    }

    goldmap= (GameBoard*)mmap(NULL, no_of_row*no_of_col+sizeof(GameBoard), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    goldmap->rows=no_of_row;
    goldmap->cols=no_of_col;
    goldmap->daemonID=0;

    const char *theMine=&m[0];
    const char* ptr=theMine;
    unsigned char* mp=(unsigned char *)goldmap->map;

    for(int i=0;i<5;i++)
    {
      goldmap->pid[i]=0;
    }

    //Convert the ASCII bytes into bit fields drawn from goldchase.h
    while(*ptr!='\0')
    {
      if(*ptr==' ')      *mp=0;
      else if(*ptr=='*') *mp=G_WALL; //A wall
      ++ptr;
      ++mp;
    }

    srand (time(NULL));
    while(check!=number_of_gold)
    {
      fool = rand() % mul;
      if((check<number_of_gold-1) && (goldmap->map[fool]==0))
      {
        goldmap->map[fool]=G_FOOL;
        check++;
      }
      if((check==number_of_gold-1) && (goldmap->map[fool]==0))
      {
        goldmap->map[fool]=G_GOLD;
        check++;
      }

    }   //end of while

    check=0;

    current_player=G_PLR0;
    goldmap->pid[0]=getpid();
    while(check !=1)  //place first player
    {
      plyr = rand() % (no_of_row*no_of_col);
      if(goldmap->map[plyr]==0)
      {
        goldmap->map[plyr]=current_player;
        check++;
      }


    }

    readQueue(msg_q[0]);

    sem_post(first_semaphore); //release the semaphore


  }
  else //for subsequent players
  {
    DEBUG("Begin \"else //for subsequent players\"\n");
    first_semaphore=sem_open("/multiplayergames",O_RDWR);

    fd=shm_open("/shared_map", O_RDWR,0);
    if(fd==-1)
    {
      perror("Opening shared memory ");

    }
    int rows1;
    int cols1;
    read(fd, &rows1, sizeof(int));
    read(fd, &cols1, sizeof(int));
    goldmap= (GameBoard*)mmap(NULL, rows1*cols1+sizeof(GameBoard),
        PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

    goldmap->rows=rows1;
    goldmap->cols=cols1;


    DEBUG("client game process and/or subsequent player game process after mmap\n");

    if((goldmap->pid[0]==0) && (goldmap->players & G_PLR0)==0)
    {
      cnt = 0;
      current_player=G_PLR0;
      readQueue(msg_q[0]);

    }
    else if((goldmap->pid[1]==0))// && (goldmap->players & G_PLR1)==0)
    {
      cnt = 1;
      current_player=G_PLR1;
      readQueue(msg_q[1]);
    }
    else if((goldmap->pid[2]==0))// && (goldmap->players & G_PLR2)==0)
    {
      cnt = 2;
      current_player=G_PLR2;
      readQueue(msg_q[2]);
    }
    else if((goldmap->pid[3]==0))// && (goldmap->players & G_PLR3)==0)
    {
      cnt = 3;
      current_player=G_PLR3;
      readQueue(msg_q[3]);
    }
    else if((goldmap->pid[4]==0))// && (goldmap->players & G_PLR4)==0)
    {
      cnt = 4;
      current_player=G_PLR4;
      readQueue(msg_q[4]);
    }
    else
    {
      cout<<"All players on board"<<endl;
      return 0;
    }

    sem_wait(first_semaphore);
    for(int i=0;i<5;i++)
    {
      if(goldmap->pid[i]==0)
      {
        goldmap->pid[i]=getpid();
        break;
      }
    }
    sem_post(first_semaphore);

    check=0;
    while(check !=1)  //place all players one by one
    {
      plyr = rand() % (rows1*cols1);
      if(goldmap->map[plyr]==0)
      {
        if(sem_wait(first_semaphore)==-1) //sem wait
        {
          perror("Wait value :");
        }

        goldmap->map[plyr]=current_player;

        if(sem_post(first_semaphore)==-1)
        {
          perror("Post value :");
        }
        check++;

      }

    }

    DEBUG("End \"else //for subsequent players\"\n");
  }  //end of else

  Map goldMine((unsigned char *)goldmap->map,goldmap->rows,goldmap->cols);
  ptr = &goldMine;
  sem_wait(first_semaphore);
  for(int i=0;i<5;i++)
  {
    if(goldmap->pid[i]!=0 && goldmap->pid[i] != getpid())
    {
      kill(goldmap->pid[i],SIGUSR1);
    }

  }
  sem_post(first_semaphore);
  int a=0;
  int b=0;

  if(goldmap->daemonID == 0)
  {
    create_daemon();
    //DEBUG("DEAMON ID : %d",&goldmap->daemonID);
  }
  else
  {
    kill(goldmap->daemonID,SIGHUP);
  }


  while(true && check_signal == false)
  {
    set = false;
    a=goldMine.getKey();
    calc_col = plyr % goldmap->cols;
    calc_row = plyr / goldmap->cols;
    if(a=='h')  //left
    {
      if(calc_col != 0)
      {
        if(!(goldmap->map[plyr-1] & G_WALL))
        {
          if(sem_wait(first_semaphore)==-1) //sem wait
          {
            perror("Wait value :");
          }

          goldmap->map[plyr]&= ~current_player;//~G_PLR0;
          goldmap->map[plyr-1]|= current_player;//G_PLR0;
          plyr = plyr -1;
          goldMine.drawMap();
          set = true;
          if(sem_post(first_semaphore)==-1)
          {
            perror("Post value :");
          }
        }
        kill(goldmap->daemonID,SIGUSR1);
      }
      else
      {
        if(found==true)
        {
          sem_wait(first_semaphore);
          goldmap->map[plyr]&= ~current_player;
          sem_post(first_semaphore);
          break;
        }
        else
          goldMine.postNotice("Cannot leave the board ");
      }

    }
    else if(a=='l') //right
    {
      if(found==true && calc_col>=goldmap->cols-1)
      {
        sem_wait(first_semaphore);
        goldmap->map[plyr]&= ~current_player;
        sem_post(first_semaphore);
        break;
      }
      if(calc_col != goldmap->cols)
      {

        if(!(goldmap->map[plyr+1] & G_WALL))
        {
          if(sem_wait(first_semaphore)==-1) //sem wait
          {
            perror("Wait value :");
          }
          goldmap->map[plyr]&= ~current_player;//G_PLR0;
          goldmap->map[plyr+1]|=current_player;//G_PLR0;
          plyr = plyr +1;
          goldMine.drawMap();
          set = true;
          sem_post(first_semaphore);
        }
        //kill(goldmap->daemonID,SIGUSR1);
      }
      else
      {

        goldMine.postNotice("Cannot leave the board ");
      }

    }
    else if(a=='j')  //down
    {
      if(calc_row < goldmap->rows-1)//(!(goldmap->map[plyr+no_of_col] & G_WALL))
      {
        if((goldmap->map[plyr+goldmap->cols] != G_WALL))
        {
          if(sem_wait(first_semaphore)==-1) //sem wait
          {
            perror("Wait value :");
          }
          goldmap->map[plyr]&= ~current_player;//G_PLR0;
          goldmap->map[plyr+goldmap->cols]|=current_player;//G_PLR0;
          plyr = plyr +goldmap->cols;
          goldMine.drawMap();
          set = true;
          sem_post(first_semaphore);
        }
        //kill(goldmap->daemonID,SIGUSR1);
      }
      else
      {
        if(found==true)
        {
          sem_wait(first_semaphore);
          goldmap->map[plyr]&= ~current_player;
          sem_post(first_semaphore);
          break;
        }
        else
          goldMine.postNotice("Cannot leave the board ");
      }

    }
    else if(a=='k') //up
    {
      if(plyr-goldmap->cols>0)
      {
        if((goldmap->map[plyr-goldmap->cols] != G_WALL)) //(!(goldmap->map[plyr-no_of_col] & G_WALL))
        {
          if(sem_wait(first_semaphore)==-1) //sem wait
          {
            perror("Wait value :");
          }
          goldmap->map[plyr]&= ~current_player;//G_PLR0;
          goldmap->map[plyr-goldmap->cols] |=current_player;//G_PLR0;
          plyr = plyr -  goldmap->cols;
          goldMine.drawMap();
          set = true;
          sem_post(first_semaphore);
        }

        // kill(goldmap->daemonID,SIGUSR1);
      }
      else
      {
        if(found==true)
        {
          sem_wait(first_semaphore);
          goldmap->map[plyr]&= ~current_player;
          sem_post(first_semaphore);
          break;
        }
        else
          goldMine.postNotice("Cannot leave the board ");
      }

    }
    else if(a=='b')
    {
      char message_text[121];
      memset(message_text, 0, 121);
      string p;
      if(current_player == G_PLR0)
      {
        p="Player 1 says:";
      }
      else if(current_player == G_PLR1)
      {
        p="Player 2 says:";
      }
      else if(current_player == G_PLR2)
      {
        p="Player 3 says:";
      }
      else if(current_player == G_PLR3)
      {
        p="Player 4 says:";
      }
      else if(current_player == G_PLR4)
      {
        p="Player 5 says:";
      }
      string msg = p+ptr->getMessage();
      strncpy(message_text, msg.c_str(), 120);
      for(int i=0;i<5;i++)
      {
        if(goldmap->pid[i]!=0 && goldmap->pid[i]!=getpid())
        {
          mqd_t writequeue_fd; //message queue file descriptor
          if((writequeue_fd=mq_open(msg_q[i].c_str(), O_WRONLY|O_NONBLOCK))==-1)
          {
            perror("mq_open");
            exit(1);
          }

          //here, build the message for "won game" (same for broadcast)
          if(mq_send(writequeue_fd, message_text, strlen(message_text), 0)==-1)
          {
            perror("mq_send");
            exit(1);
          }
          mq_close(writequeue_fd);
        }
      }
    }
    else if(a=='m')
    {
      unsigned char mask=0;
      for(int i=0;i<5;i++)
      {
        if(goldmap->pid[i]!=0 && goldmap->pid[i]!=getpid())
        {
          switch(i)
          {
            case 0:
              mask|=G_PLR0;
              break;
            case 1:
              mask|=G_PLR1;
              break;
            case 2:
              mask|=G_PLR2;
              break;
            case 3:
              mask|=G_PLR3;
              break;
            case 4:
              mask|=G_PLR4;
              break;
          }
        }
      }
      unsigned char answer=ptr->getPlayer(mask);
      char message_text[121];
      memset(message_text, 0, 121);
      string p;
      if(current_player == G_PLR0)
      {
        p="Player 1 says:";
      }
      else if(current_player == G_PLR1)
      {
        p="Player 2 says:";
      }
      else if(current_player == G_PLR2)
      {
        p="Player 3 says:";
      }
      else if(current_player == G_PLR3)
      {
        p="Player 4 says:";
      }
      else if(current_player == G_PLR4)
      {
        p="Player 5 says:";
      }
      string msg = p+ptr->getMessage();
      strncpy(message_text, msg.c_str(), 120);

      if(answer==G_PLR0)
      {
        writeQueue(msg_q[0],message_text);
      }
      else if(answer==G_PLR1)
      {
        writeQueue(msg_q[1],message_text);
      }
      else if(answer==G_PLR2)
      {
        writeQueue(msg_q[2],message_text);
      }
      else if(answer==G_PLR3)
      {
        writeQueue(msg_q[3],message_text);
      }
      else if(answer==G_PLR4)
      {
        writeQueue(msg_q[4],message_text);
      }
    }
    else if(a=='Q')
    {
      if(sem_wait(first_semaphore)==-1) //sem wait
      {
        perror("Wait value :");
      }
      DEBUG("QUIT");
      goldmap->map[plyr] &= ~current_player;
      goldMine.drawMap();
      sem_post(first_semaphore);
      break;

    }
    if(goldmap->map[plyr] & G_GOLD)
    {
      if(sem_wait(first_semaphore)==-1) //sem wait
      {
        perror("Wait value :");
      }
      goldMine.postNotice("Real Gold.. You win.. Please move to upper or lower edges to exit");
      goldmap->map[plyr]&= ~G_GOLD;
      sem_post(first_semaphore);
      found=true;

    }
    if(goldmap->map[plyr] & G_FOOL)
    {
      if(sem_wait(first_semaphore)==-1) //sem wait
      {
        perror("Wait value :");
      }
      goldmap->map[plyr]&= ~G_FOOL;
      goldMine.postNotice("FOOL Gold ");
      sem_post(first_semaphore);
    }

    if(set==true)
    {
      set= false;
      for(int i=0;i<5;i++)
      {
        if(goldmap->pid[i]!=0)
        {
          kill(goldmap->pid[i],SIGUSR1);
          kill(goldmap->daemonID,SIGUSR1);
        }

      }

    }
  } //end of while

  sem_wait(first_semaphore);
  if(current_player == G_PLR0)
  {
    if(found==true)
    {
      char message_text[121];
      memset(message_text, 0, 121);
      strncpy(message_text,"Player 1 won ..All exit", 120);
      for(int i=0;i<5;i++)
      {
        if(goldmap->pid[i]!=0 && goldmap->pid[i]!=getpid())
        {
          mqd_t writequeue_fd; //message queue file descriptor
          if((writequeue_fd=mq_open(msg_q[i].c_str(), O_WRONLY|O_NONBLOCK))==-1)
          {
            perror("mq_open");
            exit(1);
          }

          //here, build the message for "won game" (same for broadcast)
          if(mq_send(writequeue_fd, message_text, strlen(message_text), 0)==-1)
          {
            perror("mq_send");
            exit(1);
          }
          mq_close(writequeue_fd);
        }
      }

    }
    mq_close(readqueue_fd);
    mq_unlink(msg_q[0].c_str());
    goldmap->pid[0]=0;
  }
  else if(current_player == G_PLR1)
  {
    if(found==true)
    {
      char message_text[121];
      memset(message_text, 0, 121);
      strncpy(message_text,"Player 2 won ..All exit", 120);
      for(int i=0;i<5;i++)
      {
        if(goldmap->pid[i]!=0 && goldmap->pid[i]!=getpid())
        {
          mqd_t writequeue_fd; //message queue file descriptor
          if((writequeue_fd=mq_open(msg_q[i].c_str(), O_WRONLY|O_NONBLOCK))==-1)
          {
            perror("mq_open");
            exit(1);
          }

          if(mq_send(writequeue_fd, message_text, strlen(message_text), 0)==-1)
          {
            perror("mq_send");
            exit(1);
          }
          mq_close(writequeue_fd);
        }
      }

    }
    //cout<<"player 2 exited"<<endl;
    mq_close(readqueue_fd);
    mq_unlink(msg_q[1].c_str());
    goldmap->pid[1]=0;
  }
  else if(current_player == G_PLR2)
  {
    //cout<<"player 3 exited"<<endl;
    if(found==true)
    {
      char message_text[121];
      memset(message_text, 0, 121);
      strncpy(message_text,"Player 3 won ..All exit", 120);
      for(int i=0;i<5;i++)
      {
        if(goldmap->pid[i]!=0 && goldmap->pid[i]!=getpid())
        {
          mqd_t writequeue_fd; //message queue file descriptor
          if((writequeue_fd=mq_open(msg_q[i].c_str(), O_WRONLY|O_NONBLOCK))==-1)
          {
            perror("mq_open");
            exit(1);
          }

          //here, build the message for "won game" (same for broadcast)
          if(mq_send(writequeue_fd, message_text, strlen(message_text), 0)==-1)
          {
            perror("mq_send");
            exit(1);
          }
          mq_close(writequeue_fd);
        }
      }

    }
    mq_close(readqueue_fd);
    mq_unlink(msg_q[2].c_str());
    goldmap->pid[2]=0;
  }
  else if(current_player == G_PLR3)
  {
    //cout<<"player 4 exited"<<endl;
    if(found==true)
    {
      char message_text[121];
      memset(message_text, 0, 121);
      strncpy(message_text,"Player 4 won ..All exit", 120);
      for(int i=0;i<5;i++)
      {
        if(goldmap->pid[i]!=0 && goldmap->pid[i]!=getpid())
        {
          mqd_t writequeue_fd; //message queue file descriptor
          if((writequeue_fd=mq_open(msg_q[i].c_str(), O_WRONLY|O_NONBLOCK))==-1)
          {
            perror("mq_open");
            exit(1);
          }

          //here, build the message for "won game" (same for broadcast)
          if(mq_send(writequeue_fd, message_text, strlen(message_text), 0)==-1)
          {
            perror("mq_send");
            exit(1);
          }
          mq_close(writequeue_fd);
        }
      }

    }
    mq_close(readqueue_fd);
    mq_unlink(msg_q[3].c_str());
    goldmap->pid[3]=0;
  }
  else if(current_player == G_PLR4)
  {
    //cout<<"player 5 exited"<<endl;
    if(found==true)
    {
      char message_text[121];
      memset(message_text, 0, 121);
      strncpy(message_text,"Player 5 won ..All exit", 120);
      for(int i=0;i<5;i++)
      {
        if(goldmap->pid[i]!=0 && goldmap->pid[i]!=getpid())
        {
          mqd_t writequeue_fd; //message queue file descriptor
          if((writequeue_fd=mq_open(msg_q[i].c_str(), O_WRONLY|O_NONBLOCK))==-1)
          {
            perror("mq_open");
            exit(1);
          }

          //here, build the message for "won game" (same for broadcast)
          if(mq_send(writequeue_fd, message_text, strlen(message_text), 0)==-1)
          {
            perror("mq_send");
            exit(1);
          }
          mq_close(writequeue_fd);
        }
      }

    }
    mq_close(readqueue_fd);
    mq_unlink(msg_q[4].c_str());
    goldmap->pid[4]=0;
  }

  goldmap->map[plyr] &= ~current_player;
  for(int i=0;i<5;i++)
  {
    if(goldmap->pid[i]!=0)
    {
      kill(goldmap->pid[i],SIGUSR1);
    }
  }
  sem_post(first_semaphore);

  for(int i=0;i<5;i++)
  {
    if(goldmap->pid[i]==0)
    {
      count++;
    }
  }
  if(count==5)
  {
    sem_close(first_semaphore);
    sem_unlink("/multiplayergames");
    shm_unlink("/shared_map");
  }
  //   DEBUG("SEND SIGNAL TO SIGHUP PLAYR QUIT\n");
  kill(goldmap->daemonID,SIGHUP);
  //}
  return 0;
  }

void handle_interrupt(int)
{
  ptr->drawMap();
}


void clean_up(int)
{
  check_signal = true;
  return;
}



void create_daemon()
{
  if(fork()>0)
  {
    //I'm the parent, leave the function
    return;
  }

  if(fork()>0)
    exit(0);
  if(setsid()==-1)
    exit(1);
  for(int i=0; i< sysconf(_SC_OPEN_MAX); ++i)
  {
    if(i != debugFD)
    {
      close(i);
    }
  }
  open("/dev/null", O_RDWR); //fd 0
  open("/dev/null", O_RDWR); //fd 1
  open("/dev/null", O_RDWR); //fd 2
  umask(0);
  chdir("/");

  DEBUG("daemon created\n");

  //gm = goldmap;


  //  Sighup
  struct sigaction sighup_action;
  //handle with this function
  sighup_action.sa_handler=sig_up;
  //zero out the mask (allow any signal to interrupt)
  sigemptyset(&sighup_action.sa_mask);
  sigaddset(&sighup_action.sa_mask, SIGUSR1);
  sighup_action.sa_flags=0;
  //tell how to handle SIGHUP
  sigaction(SIGHUP, &sighup_action, NULL);


  // SIGUSR1
  struct sigaction sigusr1_action;
  //handle with this function
  sigusr1_action.sa_handler=handle_interrupt_usr;
  //zero out the mask (allow any signal to interrupt)
  sigemptyset(&sigusr1_action.sa_mask);
  sigaddset(&sigusr1_action.sa_mask, SIGHUP);
  sigusr1_action.sa_flags=0;
  //tell how to handle SIGURS1
  sigaction(SIGUSR1, &sigusr1_action, NULL);

  struct sigaction action_to_take;           //signal to read message
  action_to_take.sa_handler=read_message1;
  sigemptyset(&action_to_take.sa_mask);
  action_to_take.sa_flags=0;
  sigaction(SIGUSR2, &action_to_take, NULL);

  int status; //for error checking                                 //server socket !!!!!!!!!!!!!!!!!!

  //change this # between 2000-65k before using
  const char* portno="42424";
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints)); //zero out everything in structure
  hints.ai_family = AF_UNSPEC; //don't care. Either IPv4 or IPv6
  hints.ai_socktype=SOCK_STREAM; // TCP stream sockets
  hints.ai_flags=AI_PASSIVE; //file in the IP of the server for me

  struct addrinfo *servinfo;
  if((status=getaddrinfo(NULL, portno, &hints, &servinfo))==-1)
  {
    fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
    exit(1);
  }
  sockfd=socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

  /*avoid "Address already in use" error*/
  int yes=1;
  if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))==-1)
  {
    perror("setsockopt");
    exit(1);
  }

  //We need to "bind" the socket to the port number so that the kernel
  //can match an incoming packet on a port to the proper process
  if((status=bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen))==-1)
  {
    perror("bind");
    exit(1);
  }

  //when done, release dynamically allocated memory
  freeaddrinfo(servinfo);

  if(listen(sockfd,1)==-1)
  {
    perror("listen");
    exit(1);
  }

  printf("Blocking, waiting for client to connect\n");

  struct sockaddr_in client_addr;
  socklen_t clientSize=sizeof(client_addr);

  do
    new_sockfd=accept(sockfd, (struct sockaddr*) &client_addr, &clientSize);
  while(new_sockfd == -1 && errno == EINTR);

  sockfd = new_sockfd;
  //	WRITE(debugFD,"accepted\n",sizeof("accepted\n"));
  //read & write to the socket

  sock_q[0]="/game_player1_mq";
  sock_q[1]="/game_player2_mq";
  sock_q[2]="/game_player3_mq";
  sock_q[3]="/game_player4_mq";
  sock_q[4]="/game_player5_mq";


  int rows =0;
  int cols = 0;

  rows = goldmap->rows;
  cols = goldmap->cols;

  local_copy = new unsigned char[(rows*cols)];

  //unsigned char local_mapcopy[rows*cols];

  goldmap->daemonID = getpid();

  for(int x = 0; x < rows*cols ; x++)
  {
    local_copy[x] = goldmap->map[x];
  }

  char buffer[100];
  memset(buffer,0,100);
  int n;
  WRITE(new_sockfd,&rows,sizeof(rows));
  WRITE(new_sockfd,&cols,sizeof(cols));
  if(rows == 26)
    DEBUG("acceptedrows\n");
  if(cols == 80)
    DEBUG("acceptedcols\n");
  unsigned char* mptr = local_copy;

  for(int i=0;i<(rows*cols);i++)
  {
    unsigned char temp = local_copy[i];

    if(temp == G_WALL)
      DEBUG("*");
    if(temp == 0)
      DEBUG(" ");

    WRITE(new_sockfd,&temp,sizeof(temp));

  }


  unsigned char temp=G_SOCKPLR;
  for(int i=0;i<5;i++)
  {
    if(goldmap->pid[i]!=0)
    {
      if(i==0)
      {
        temp|= G_PLR0;
      }
      else if(i==1)
      {
        temp|= G_PLR1;
      }
      else if(i==2)
      {
        temp|= G_PLR2;
      }
      else if(i==3)
      {
        temp|= G_PLR3;
      }
      else if(i==4)
      {
        temp|= G_PLR4;
      }
    }
    //G_SOCKPLR |=temp;


  }
  WRITE(new_sockfd,&temp,sizeof(temp));

  continous_while();
  //close(new_sockfd);

}



void client_daemon(const char *s)
{

  DEBUG("Entering client_daemon()\n");
  pipe(pipefd);

  if(fork()>0)
  {
    //I'm the parent, leave the function

    close(pipefd[1]); //close write, parent only needs read
    int val;
    READ(pipefd[0], &val, sizeof(val));

    if(val==1)
      write(1, "Success!\n", sizeof("Success!\n"));
    else
      write(1, "Failure!\n", sizeof("Failure!\n"));
    DEBUG("Client game process released to continue!\n");
    return;
  }
  DEBUG("Client daemon about to do second fork()\n");

  if(fork()>0)
    exit(0);
  if(setsid()==-1)
    exit(1);
  DEBUG("About to close file descriptors in client daemon\n");
  for(int i=0; i< sysconf(_SC_OPEN_MAX); ++i)
  {
    if(i!=pipefd[1] && i!=debugFD )
    {
      close(i);
    }
  }
  DEBUG("Finished closing file descriptors in client daemon\n");
  open("/dev/null", O_RDWR); //fd 0
  open("/dev/null", O_RDWR); //fd 1
  open("/dev/null", O_RDWR); //fd 2
  umask(0);
  chdir("/");
  //sleep(5); //here, daemon is setting up shared memory


  DEBUG("location: client daemon created\n");


  //  Sighup
  struct sigaction sighup_action;
  //handle with this function
  sighup_action.sa_handler=sig_up;
  //zero out the mask (allow any signal to interrupt)
  sigemptyset(&sighup_action.sa_mask);
  sighup_action.sa_flags=0;
  //tell how to handle SIGHUP
  sigaction(SIGHUP, &sighup_action, NULL);


  // SIGUSR1
  struct sigaction sigusr1_action;
  //handle with this function
  sigusr1_action.sa_handler=handle_interrupt_usr;
  //zero out the mask (allow any signal to interrupt)
  sigemptyset(&sigusr1_action.sa_mask);
  sigusr1_action.sa_flags=0;
  //tell how to handle SIGURS1
  sigaction(SIGUSR1, &sigusr1_action, NULL);


  struct sigaction action_to_take;           //signal to read message
  action_to_take.sa_handler=read_message1;
  sigemptyset(&action_to_take.sa_mask);
  action_to_take.sa_flags=0;
  sigaction(SIGUSR2, &action_to_take, NULL);

  const char* portno="42424";

  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints)); //zero out everything in structure
  hints.ai_family = AF_UNSPEC; //don't care. Either IPv4 or IPv6
  hints.ai_socktype=SOCK_STREAM; // TCP stream sockets

  struct addrinfo *servinfo;
  //instead of "localhost", it could by any domain name  "192.168.246.141"
  if((status=getaddrinfo(s, portno, &hints, &servinfo))==-1)
  {
    fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
    exit(1);
  }
  sockfd=socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

  if((status=connect(sockfd, servinfo->ai_addr, servinfo->ai_addrlen))==-1)
  {
    perror("connect");
    exit(1);
  }
  //release the information allocated by getaddrinfo()

  new_sockfd = sockfd;

  sock_q[0]="/game_player1_mq";
  sock_q[1]="/game_player2_mq";
  sock_q[2]="/game_player3_mq";
  sock_q[3]="/game_player4_mq";
  sock_q[4]="/game_player5_mq";


  freeaddrinfo(servinfo);
  DEBUG("location: client daemon created 2\n");

  const char* message="One small step for (a) man, one large  leap for Mankind";
  int rows, cols;
  READ(new_sockfd, &rows, sizeof(rows));
  READ(new_sockfd, &cols, sizeof(cols));

  local_copy = new unsigned char[(rows*cols)];

  for(int x = 0; x < rows*cols ; x++)
  {
    READ(new_sockfd,&local_copy[x],sizeof(local_copy[x]));
  }


  DEBUG("location: client daemon created 3\n");

  //sem_t *client_semaphore;   //step 4
  first_semaphore=sem_open("/multiplayergames",O_CREAT|O_EXCL|O_RDWR,S_IROTH| S_IWOTH| S_IRGRP| S_IWGRP| S_IRUSR| S_IWUSR,1);
  if(first_semaphore==SEM_FAILED)
  {
    if(errno!=EEXIST)
    {
      perror("Opening Semaphore :");
      exit(1);
    }

  }

  int fd1;
  fd1=shm_open("/shared_map", O_RDWR|O_CREAT|O_EXCL, S_IRUSR|S_IWUSR);  //step 5
  if(fd1 == -1)
  {
    perror("Opening shared memory ");
  }


  int trunc1;
  trunc1=ftruncate(fd1,(cols*rows)+sizeof(GameBoard));

  if(trunc1==-1)
  {
    perror("File truncate :");
  }

  //step 6
  goldmap= (GameBoard*)mmap(NULL,(rows*cols)+sizeof(GameBoard), PROT_READ|PROT_WRITE, MAP_SHARED, fd1, 0);
  goldmap->rows=rows;
  goldmap->cols=cols;

  goldmap->daemonID = getpid();  //step 7

  for(int i=0;i<(rows*cols);i++)
  {
    if(local_copy[i] == G_WALL)
      DEBUG("*");
    if(local_copy[i] == 0)
      DEBUG(" ");
    goldmap -> map[i] = local_copy[i];
  }


  DEBUG("flow is coming here\n");


  unsigned char sock_p;
  READ(new_sockfd,&sock_p,sizeof(sock_p));
  DEBUG("sock_p=%d\n",sock_p);



  //change this # between 2000-65k before using


  unsigned char p_bit[5] = {G_PLR0,G_PLR1,G_PLR2,G_PLR3,G_PLR4};
  for(int i=0;i<5;i++)
  {
    if(sock_p & p_bit[i] && goldmap->pid[i]==0)
    {
      DEBUG("client daemon setting up mq for player %d\n",i);
      struct mq_attr mq_attributes;
      mq_attributes.mq_flags=0;
      mq_attributes.mq_maxmsg=10;
      mq_attributes.mq_msgsize=120;

      if((sock_readqueue[i]=mq_open(sock_q[i].c_str(), O_RDONLY|O_CREAT|O_EXCL|O_NONBLOCK,
              S_IRUSR|S_IWUSR, &mq_attributes))==-1)
      {
        perror("mq_open drftgh");
        exit(1);
      }

      struct sigevent mq_notification_event; //set up message queue to receive signal whenever message comes in
      mq_notification_event.sigev_notify=SIGEV_SIGNAL;
      mq_notification_event.sigev_signo=SIGUSR2;
      mq_notify(sock_readqueue[i], &mq_notification_event);

      goldmap->pid[i] = goldmap->daemonID;
      //here create message queue for player i who is on server
    }

  }

  int val=1;
  write(pipefd[1], &val, sizeof(val));

  DEBUG("Client daemon about to enter continuous_while()\n");
  continous_while();

  //	close(sockfd);
}


void continous_while()
{

  while(1)
  {
    unsigned char byte;
    DEBUG("in daemon while(1), waiting to read character\n");
    READ(new_sockfd,&byte,1);
    if(byte & G_SOCKPLR)
    {
      DEBUG("just read in G_SOCKPLR\n");
      unsigned char p_bit[5] = {G_PLR0,G_PLR1,G_PLR2,G_PLR3,G_PLR4};
      for(int i=0;i<5;i++)
      {
        if(byte  & p_bit[i] && goldmap->pid[i]==0)
        {
          struct mq_attr mq_attributes;
          mq_attributes.mq_flags=0;
          mq_attributes.mq_maxmsg=10;
          mq_attributes.mq_msgsize=120;


          if((sock_readqueue[i]=mq_open(sock_q[i].c_str(), O_RDONLY|O_CREAT|O_EXCL|O_NONBLOCK,
                  S_IRUSR|S_IWUSR, &mq_attributes))==-1)
          {
            perror("mq_open drftgh");
            exit(1);
          }

          struct sigevent mq_notification_event; //set up message queue to receive signal whenever message comes in
          mq_notification_event.sigev_notify=SIGEV_SIGNAL;
          mq_notification_event.sigev_signo=SIGUSR2;
          mq_notify(sock_readqueue[i], &mq_notification_event);

          goldmap->pid[i] = goldmap->daemonID;
        }
        else if(!(byte  & p_bit[i]) && goldmap->pid[i]!=0)
        {
          mq_close(sock_readqueue[i]);
          mq_unlink(sock_q[i].c_str());
          goldmap->pid[i]=0;
        }
      }
    }
    if(byte==G_SOCKPLR)
    {
      DEBUG("LAST PLAYER");
      kill(goldmap->daemonID,SIGHUP);
    }

    if(byte & G_SOCKMSG)
    {
      DEBUG("just read in G_SOCKMSG\n");
      unsigned char temp =0;
      unsigned char p_bit[5] = {G_PLR0,G_PLR1,G_PLR2,G_PLR3,G_PLR4};
      for(int i=0;i<5;i++)
      {
        if(byte  & p_bit[i] )//&& goldmap->pid[i]==0)
        {   
          mqd_t writequeue_fd; //message queue file descriptor
          if((writequeue_fd=mq_open(sock_q[i].c_str(), O_WRONLY|O_NONBLOCK))==-1)
          {
            perror("mq_open");
            exit(1);
          }

          int l;
          char message[121];
          memset(message,0,121);
          READ(new_sockfd,&l,sizeof(l));
          DEBUG("l = %d\n",l);
          for(int i=0;i<l;i++)
          {
            READ(new_sockfd,&message[i],sizeof(message[i]));
            DEBUG("%c",message[i]);
          }
          DEBUG("\n");

          if(mq_send(writequeue_fd, message, strlen(message), 0)==-1)
          {
            perror("mq_send");
            exit(1);
          }                  
          mq_close(writequeue_fd);
        }
      }

    }
    if(byte==0)//map has changed!
    {
      DEBUG("just read in 0 (map change)\n");
      short n = size;
      READ(new_sockfd, &n, sizeof(n));
      for(int i=0; i<n; ++i)
      {
        short a;
        unsigned char p;
        READ(new_sockfd,&a,sizeof(a));
        READ(new_sockfd,&p,sizeof(p));
        local_copy[a]=p;
        goldmap->map[a]=p;
        for(int i=0;i<5;i++)
        {
          if(goldmap->pid[i]!=0 && goldmap->pid[i]!=getpid())
          {
            kill(goldmap->pid[i], SIGUSR1);
          }
        }
      }

    }

  } //infinite loop


}


void handle_interrupt_usr(int)
{
  DEBUG("Entering daemon signal handler: handle_interrupt_usr()\n");   
  unsigned char c=0;
  vector< pair<short,unsigned char> > pvec;
  for(short i=0; i<goldmap->rows*goldmap->cols; ++i)
  {
    if(goldmap->map[i]!=local_copy[i])
    {
      pair<short,unsigned char> aPair;
      aPair.first=i;
      aPair.second=goldmap->map[i];
      pvec.push_back(aPair);
      local_copy[i]=goldmap->map[i];
    }
  }
  sock_byte = c;

  DEBUG("  In handle_interrupt_usr(), about to write map changes\n");   
  WRITE(new_sockfd,&sock_byte,sizeof(sock_byte));
  size = pvec.size();
  WRITE(new_sockfd,&size,sizeof(size));
  for(int i= 0;i<pvec.size();i++)
  {
    WRITE(new_sockfd,&pvec[i].first,sizeof(pvec[i].first));
    WRITE(new_sockfd,&pvec[i].second,sizeof(pvec[i].second));
  }
  DEBUG("  exiting handle_interrupt_usr()\n");   
}

void read_message1(int)
{
  DEBUG("Entering signal handler: read_message1()\n");   

  int err;
  unsigned char msg_byte=0;
  char msg[121];
  memset(msg, 0, 121);//set all characters to '\0'

  int msglength;   

  for(int i=0;i<5;i++)
  {

    if(goldmap->pid[i]==getpid())
    {
      struct sigevent mq_notification_event;
      mq_notification_event.sigev_notify=SIGEV_SIGNAL;
      mq_notification_event.sigev_signo=SIGUSR2;

      mq_notify(sock_readqueue[i], &mq_notification_event);
      if((err=mq_receive(sock_readqueue[i], msg, 120, NULL))!=-1)
      {
        if(i==0)
        {
          msg_byte|= G_PLR0;
        }
        else if(i==1)
        {
          msg_byte|= G_PLR1;
        }
        else if(i==2)
        {
          msg_byte|= G_PLR2;

        }
        else if(i==3)
        {
          msg_byte|= G_PLR3;

        }
        else if(i==4)
        {
          msg_byte|= G_PLR0;
        }  

      }  
      DEBUG("About to write G_SOCKMSG for i=%d \n",i);   
      WRITE(new_sockfd,&(msg_byte|=G_SOCKMSG),sizeof(msg_byte|=G_SOCKMSG));	

      msglength = strlen(msg);
      DEBUG("%d : \n",msglength);
      WRITE(new_sockfd,&msglength,sizeof(msglength));

      for(int i=0;i<msglength;i++)
      {
        WRITE(new_sockfd,&msg[i],sizeof(msg[i]));
        DEBUG("%c :",msg[i]);
      }
      DEBUG("\n");
    }
  }
  DEBUG("Leaving signal handler: read_message1()\n");   
}


void sig_up(int)
{
  DEBUG("In sig_up(), the SIGHUP signal handler for daemon\n");	
  DEBUG("  going to send G_SOCKPLR across socket.\n");	
  temp3=G_SOCKPLR;
  for(int i=0;i<5;i++)
  {
    if(goldmap->pid[i]!=0)
    {
      if(i==0)
      {
        temp3|= G_PLR0;
      }
      else if(i==1)
      {
        temp3|= G_PLR1;
      }
      else if(i==2)
      {
        temp3|= G_PLR2;
      }
      else if(i==3)
      {
        temp3|= G_PLR3;
      }
      else if(i==4)
      {
        temp3|= G_PLR4;
      }
    }
  }
  sock_byte = temp3; //this line does nothing useful
  DEBUG("  in sig_up(), about to write byte to socket\n");
  WRITE(new_sockfd,&temp3,sizeof(temp3));
  if(temp3 ==G_SOCKPLR)
  {
    DEBUG("No player");		
    sem_close(first_semaphore);
    sem_unlink("/multiplayergames");
    shm_unlink("/shared_map");
    delete [] local_copy;
    exit(1);
  }
  DEBUG("  leaving sig_up(), the SIGHUP signal handler for daemon.\n");
}

void read_message(int)
{
  struct sigevent mq_notification_event;
  mq_notification_event.sigev_notify=SIGEV_SIGNAL;
  mq_notification_event.sigev_signo=SIGUSR2;
  mq_notify(readqueue_fd, &mq_notification_event);

  int err;
  char msg[121];
  memset(msg, 0, 121);//set all characters to '\0'
  while((err=mq_receive(readqueue_fd, msg, 120, NULL))!=-1)
  {
    DEBUG("game process reading message length=%lu\n",strlen(msg));
    DEBUG("message is->%s<-\n",msg);
    ptr->postNotice(msg);
    memset(msg, 0, 121);//set all characters to '\0'
  }

}




void readQueue(string qName)
{
  struct mq_attr mq_attributes;
  mq_attributes.mq_flags=0;
  mq_attributes.mq_maxmsg=10;
  mq_attributes.mq_msgsize=120;

  if((readqueue_fd=mq_open(qName.c_str(), O_RDONLY|O_CREAT|O_EXCL|O_NONBLOCK,
          S_IRUSR|S_IWUSR, &mq_attributes))==-1)
  {
    perror("mq_open drftgh");
    exit(1);
  }

  struct sigevent mq_notification_event; //set up message queue to receive signal whenever message comes in
  mq_notification_event.sigev_notify=SIGEV_SIGNAL;
  mq_notification_event.sigev_signo=SIGUSR2;
  mq_notify(readqueue_fd, &mq_notification_event);
}

void writeQueue(string wName,char *message_text)
{
  mqd_t writequeue_fd; //message queue file descriptor
  if((writequeue_fd=mq_open(wName.c_str(), O_WRONLY|O_NONBLOCK))==-1)
  {
    perror("mq_open");
    exit(1);
  }

  //here, build the message for "won game" (same for broadcast)
  if(mq_send(writequeue_fd, message_text, strlen(message_text), 0)==-1)
  {
    perror("mq_send");
    exit(1);
  }
  mq_close(writequeue_fd);
}
