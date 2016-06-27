#include<stdio.h>
#include<string.h>
#include<sqlca.h>
 
EXEC SQL BEGIN DECLARE SECTION;
#define info_len 3 
struct information
{
    unsigned long int id;
    char name[36];
}info[info_len];
 
int insert_record=0;//记录一次插入表格的行数
int student_count=0;//统计student表格的行数
int i=0,j=0,m=0;
 
char   tt_username[20];   
char   tt_password[20]; 
char   tt_server[20];                    
char   tt_temp[20]="";
 
EXEC SQL END DECLARE SECTION;
int out_for_count=0;
int for_count=0;//记录for循环的次数
int fetch_record=0;//记录一次fetch影响的行数
 
int main()
{
  strcpy(tt_username,"student");
  strcpy(tt_password,"myword");
  strcpy(tt_server,"car");
 
  EXEC SQL CONNECT :tt_username IDENTIFIED BY :tt_password using :tt_server;   /*连接数据库*/
  
 
//for(out_for_count=0;out_for_count<3;out_for_count++)
{       
 
            EXEC SQL select count(*) into :student_count from student;
            printf("student 表格共有%d 行数据.\n",student_count);
                 EXEC SQL DECLARE sel_tt CURSOR FOR
                         SELECT * FROM STUDENT;
            EXEC SQL OPEN sel_tt;
            i=0;
            fetch_record=0;
            insert_record=0;
            sqlca.sqlerrd[2]=0;
            for(m=0;m<student_count/info_len+1;m++)
            {   
                i=fetch_record;
                EXEC SQL  FETCH  sel_tt INTO :info;                    
                fetch_record=sqlca.sqlerrd[2];
                insert_record = fetch_record-i;                     
                for(j=0;j<insert_record;j++)
                {
                     printf("\nid=%d,\tname=%s",info[j].id,info[j].name);
                }
                EXEC SQL FOR :insert_record INSERT INTO stu values(:info);
                 for(j=0;j<info_len;j++)
                {
                    info[j].id=0;
                    strcpy(info[j].name,"");
                }
            }
            EXEC SQL close sel_tt;
            EXEC SQL commit;
 
}
return 0;
}