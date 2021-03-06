/************************************************************
	Project#1:	CLP & DDL
 ************************************************************/

#include "db.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>

int main(int argc, char** argv)
{
	int rc = 0;
	token_list *tok_list=NULL, *tok_ptr=NULL, *tmp_tok_ptr=NULL;


	if ((argc != 2) || (strlen(argv[1]) == 0))
	{
		printf("Usage: db \"command statement\"");
		return 1;
	}

	rc = initialize_tpd_list();

  if (rc)
  {
		printf("\nError in initialize_tpd_list().\nrc = %d\n", rc);
  }
	else
	{
    rc = get_token(argv[1], &tok_list);

		/* Test code */
		tok_ptr = tok_list;
		while (tok_ptr != NULL)
		{
			// printf("%16s \t%d \t %d\n",tok_ptr->tok_string, tok_ptr->tok_class, tok_ptr->tok_value);
			tok_ptr = tok_ptr->next;
		}
    
		if (!rc)
		{
			rc = do_semantic(tok_list);
		}

		if (rc)
		{
			tok_ptr = tok_list;
			while (tok_ptr != NULL)
			{
				if ((tok_ptr->tok_class == error) ||
					  (tok_ptr->tok_value == INVALID))
				{
					printf("\nError in the string: %s\n", tok_ptr->tok_string);
					printf("rc=%d\n", rc);
					break;
				}
				tok_ptr = tok_ptr->next;
			}
		}

    /* Whether the token list is valid or not, we need to free the memory */
		tok_ptr = tok_list;
		while (tok_ptr != NULL)
		{
      tmp_tok_ptr = tok_ptr->next;
      free(tok_ptr);
      tok_ptr=tmp_tok_ptr;
		}
	}

	return rc;
}

/************************************************************* 
	This is a lexical analyzer for simple SQL statements
 *************************************************************/
int get_token(char* command, token_list** tok_list)
{
	int rc=0,i,j;
	char *start, *cur, temp_string[MAX_TOK_LEN];
	bool done = false;
	
	start = cur = command;
	while (!done)
	{
		bool found_keyword = false;

		/* This is the TOP Level for each token */
	  memset ((void*)temp_string, '\0', MAX_TOK_LEN);
		i = 0;

		/* Get rid of all the leading blanks */
		while (*cur == ' ')
			cur++;

		if (cur && isalpha(*cur))
		{
			// find valid identifier
			int t_class;
			do 
			{
				temp_string[i++] = *cur++;
			}
			while ((isalnum(*cur)) || (*cur == '_'));

			if (!(strchr(STRING_BREAK, *cur)))
			{
				/* If the next char following the keyword or identifier
				   is not a blank, (, ), or a comma, then append this
					 character to temp_string, and flag this as an error */
				temp_string[i++] = *cur++;
				add_to_list(tok_list, temp_string, error, INVALID);
				rc = INVALID;
				done = true;
			}
			else
			{

				// We have an identifier with at least 1 character
				// Now check if this ident is a keyword
				for (j = 0, found_keyword = false; j < TOTAL_KEYWORDS_PLUS_TYPE_NAMES; j++)
				{
					if ((stricmp(keyword_table[j], temp_string) == 0))
					{
						found_keyword = true;
						break;
					}
				}

				if (found_keyword)
				{
				  if (KEYWORD_OFFSET+j < K_CREATE)
						t_class = type_name;
					else if (KEYWORD_OFFSET+j >= F_SUM)
            t_class = function_name;
          else
					  t_class = keyword;

					add_to_list(tok_list, temp_string, t_class, KEYWORD_OFFSET+j);
				}
				else
				{
					if (strlen(temp_string) <= MAX_IDENT_LEN)
					  add_to_list(tok_list, temp_string, identifier, IDENT);
					else
					{
						add_to_list(tok_list, temp_string, error, INVALID);
						rc = INVALID;
						done = true;
					}
				}

				if (!*cur)
				{
					add_to_list(tok_list, "", terminator, EOC);
					done = true;
				}
			}
		}
		else if (isdigit(*cur))
		{
			// find valid number
			do 
			{
				temp_string[i++] = *cur++;
			}
			while (isdigit(*cur));

			if (!(strchr(NUMBER_BREAK, *cur)))
			{
				/* If the next char following the keyword or identifier
				   is not a blank or a ), then append this
					 character to temp_string, and flag this as an error */
				temp_string[i++] = *cur++;
				add_to_list(tok_list, temp_string, error, INVALID);
				rc = INVALID;
				done = true;
			}
			else
			{
				add_to_list(tok_list, temp_string, constant, INT_LITERAL);

				if (!*cur)
				{
					add_to_list(tok_list, "", terminator, EOC);
					done = true;
				}
			}
		}
		else if ((*cur == '(') || (*cur == ')') || (*cur == ',') || (*cur == '*')
		         || (*cur == '=') || (*cur == '<') || (*cur == '>'))
		{
			/* Catch all the symbols here. Note: no look ahead here. */
			int t_value;
			switch (*cur)
			{
				case '(' : t_value = S_LEFT_PAREN; break;
				case ')' : t_value = S_RIGHT_PAREN; break;
				case ',' : t_value = S_COMMA; break;
				case '*' : t_value = S_STAR; break;
				case '=' : t_value = S_EQUAL; break;
				case '<' : t_value = S_LESS; break;
				case '>' : t_value = S_GREATER; break;
			}

			temp_string[i++] = *cur++;

			add_to_list(tok_list, temp_string, symbol, t_value);

			if (!*cur)
			{
				add_to_list(tok_list, "", terminator, EOC);
				done = true;
			}
		}
    else if (*cur == '\'')
    {
      /* Find STRING_LITERRAL */
			int t_class;
      cur++;
			do 
			{
				temp_string[i++] = *cur++;
			}
			while ((*cur) && (*cur != '\''));

      temp_string[i] = '\0';

			if (!*cur)
			{
				/* If we reach the end of line */
				add_to_list(tok_list, temp_string, error, INVALID);
				rc = INVALID;
				done = true;
			}
      else /* must be a ' */
      {
        add_to_list(tok_list, temp_string, constant, STRING_LITERAL);
        cur++;
				if (!*cur)
				{
					add_to_list(tok_list, "", terminator, EOC);
					done = true;
        }
      }
    }
		else
		{
			if (!*cur)
			{
				add_to_list(tok_list, "", terminator, EOC);
				done = true;
			}
			else
			{
				/* not a ident, number, or valid symbol */
				temp_string[i++] = *cur++;
				add_to_list(tok_list, temp_string, error, INVALID);
				rc = INVALID;
				done = true;
			}
		}
	}
			
  return rc;
}

void add_to_list(token_list **tok_list, char *tmp, int t_class, int t_value)
{
	token_list *cur = *tok_list;
	token_list *ptr = NULL;

	// printf("%16s \t%d \t %d\n",tmp, t_class, t_value);

	ptr = (token_list*)calloc(1, sizeof(token_list));
	strcpy(ptr->tok_string, tmp);
	ptr->tok_class = t_class;
	ptr->tok_value = t_value;
	ptr->next = NULL;

  if (cur == NULL)
		*tok_list = ptr;
	else
	{
		while (cur->next != NULL)
			cur = cur->next;

		cur->next = ptr;
	}
	return;
}

int do_semantic(token_list *tok_list)
{
	int rc = 0, cur_cmd = INVALID_STATEMENT;
	bool unique = false;
  token_list *cur = tok_list;

	if ((cur->tok_value == K_CREATE) &&
			((cur->next != NULL) && (cur->next->tok_value == K_TABLE)))
	{
		printf("CREATE TABLE statement\n");
		cur_cmd = CREATE_TABLE;
		cur = cur->next->next;
	}
	else if ((cur->tok_value == K_DROP) &&
					((cur->next != NULL) && (cur->next->tok_value == K_TABLE)))
	{
		printf("DROP TABLE statement\n");
		cur_cmd = DROP_TABLE;
		cur = cur->next->next;
	}
	else if ((cur->tok_value == K_LIST) &&
					((cur->next != NULL) && (cur->next->tok_value == K_TABLE)))
	{
		printf("LIST TABLE statement\n");
		cur_cmd = LIST_TABLE;
		cur = cur->next->next;
	}
	else if ((cur->tok_value == K_LIST) &&
					((cur->next != NULL) && (cur->next->tok_value == K_SCHEMA)))
	{
		printf("LIST SCHEMA statement\n");
		cur_cmd = LIST_SCHEMA;
		cur = cur->next->next;
	}

	else if ((cur->tok_value == K_INSERT) &&
					((cur->next != NULL) && (cur->next->tok_value == K_INTO)))
	{
		printf("INSERT statement\n");
		cur_cmd = INSERT;
		cur = cur->next->next;
	}

	else if ((cur->tok_value == K_SELECT) &&
					((cur->next != NULL)))
	{
		printf("SELECT statement\n");
		cur_cmd = SELECT;
		cur = cur->next;
	}
	
	else
  {
		printf("Invalid statement\n");
		rc = cur_cmd;
	}

	if (cur_cmd != INVALID_STATEMENT)
	{
		switch(cur_cmd)
		{
			case CREATE_TABLE:
						rc = sem_create_table(cur);
						break;
			case DROP_TABLE:
						rc = sem_drop_table(cur);
						break;
			case LIST_TABLE:
						rc = sem_list_tables();
						break;
			case LIST_SCHEMA:
						rc = sem_list_schema(cur);
						break;
			case INSERT:
						rc = sem_insert(cur);
						break;
			case SELECT:
						rc = sem_select(cur);
						break;
			default:
					; /* no action */
		}
	}
	
	return rc;
}

int sem_create_table(token_list *t_list)
{
	int rc = 0;
	token_list *cur;
	tpd_entry tab_entry;
	tpd_entry *new_entry = NULL;
	bool column_done = false;
	int cur_id = 0;
	cd_entry	col_entry[MAX_NUM_COL];


	memset(&tab_entry, '\0', sizeof(tpd_entry));
	cur = t_list;
	if ((cur->tok_class != keyword) &&
		  (cur->tok_class != identifier) &&
			(cur->tok_class != type_name))
	{
		// Error
		rc = INVALID_TABLE_NAME;
		cur->tok_value = INVALID;
	}
	else
	{
		if ((new_entry = get_tpd_from_list(cur->tok_string)) != NULL)
		{
			rc = DUPLICATE_TABLE_NAME;
			cur->tok_value = INVALID;
		}
		else
		{
			strcpy(tab_entry.table_name, cur->tok_string);
			cur = cur->next;
			if (cur->tok_value != S_LEFT_PAREN)
			{
				//Error
				rc = INVALID_TABLE_DEFINITION;
				cur->tok_value = INVALID;
			}
			else
			{
				memset(&col_entry, '\0', (MAX_NUM_COL * sizeof(cd_entry)));

				/* Now build a set of column entries */
				cur = cur->next;
				do
				{
					if ((cur->tok_class != keyword) &&
							(cur->tok_class != identifier) &&
							(cur->tok_class != type_name))
					{
						// Error
						rc = INVALID_COLUMN_NAME;
						cur->tok_value = INVALID;
					}
					else
					{
						int i;
						for(i = 0; i < cur_id; i++)
						{
              /* make column name case sensitive */
							if (strcmp(col_entry[i].col_name, cur->tok_string)==0)
							{
								rc = DUPLICATE_COLUMN_NAME;
								cur->tok_value = INVALID;
								break;
							}
						}

						if (!rc)
						{
							strcpy(col_entry[cur_id].col_name, cur->tok_string);
							col_entry[cur_id].col_id = cur_id;
							col_entry[cur_id].not_null = false;    /* set default */

							cur = cur->next;
							if (cur->tok_class != type_name)
							{
								// Error
								rc = INVALID_TYPE_NAME;
								cur->tok_value = INVALID;
							}
							else
							{
                /* Set the column type here, int or char */
								col_entry[cur_id].col_type = cur->tok_value;
								cur = cur->next;
		
								if (col_entry[cur_id].col_type == T_INT)
								{
									if ((cur->tok_value != S_COMMA) &&
										  (cur->tok_value != K_NOT) &&
										  (cur->tok_value != S_RIGHT_PAREN))
									{
										rc = INVALID_COLUMN_DEFINITION;
										cur->tok_value = INVALID;
									}
								  else
									{
										col_entry[cur_id].col_len = sizeof(int);
										
										if ((cur->tok_value == K_NOT) &&
											  (cur->next->tok_value != K_NULL))
										{
											rc = INVALID_COLUMN_DEFINITION;
											cur->tok_value = INVALID;
										}	
										else if ((cur->tok_value == K_NOT) &&
											    (cur->next->tok_value == K_NULL))
										{					
											col_entry[cur_id].not_null = true;
											cur = cur->next->next;
										}
	
										if (!rc)
										{
											/* I must have either a comma or right paren */
											if ((cur->tok_value != S_RIGHT_PAREN) &&
												  (cur->tok_value != S_COMMA))
											{
												rc = INVALID_COLUMN_DEFINITION;
												cur->tok_value = INVALID;
											}
											else
		                  {
												if (cur->tok_value == S_RIGHT_PAREN)
												{
 													column_done = true;
												}
												cur = cur->next;
											}
										}
									}
								}   // end of S_INT processing
								else
								{
									// It must be char()
									if (cur->tok_value != S_LEFT_PAREN)
									{
										rc = INVALID_COLUMN_DEFINITION;
										cur->tok_value = INVALID;
									}
									else
									{
										/* Enter char(n) processing */
										cur = cur->next;
		
										if (cur->tok_value != INT_LITERAL)
										{
											rc = INVALID_COLUMN_LENGTH;
											cur->tok_value = INVALID;
										}
										else
										{
											/* Got a valid integer - convert */
											col_entry[cur_id].col_len = atoi(cur->tok_string);
											cur = cur->next;
											
											if (cur->tok_value != S_RIGHT_PAREN)
											{
												rc = INVALID_COLUMN_DEFINITION;
												cur->tok_value = INVALID;
											}
											else
											{
												cur = cur->next;
						
												if ((cur->tok_value != S_COMMA) &&
														(cur->tok_value != K_NOT) &&
														(cur->tok_value != S_RIGHT_PAREN))
												{
													rc = INVALID_COLUMN_DEFINITION;
													cur->tok_value = INVALID;
												}
												else
												{
													if ((cur->tok_value == K_NOT) &&
														  (cur->next->tok_value != K_NULL))
													{
														rc = INVALID_COLUMN_DEFINITION;
														cur->tok_value = INVALID;
													}
													else if ((cur->tok_value == K_NOT) &&
																	 (cur->next->tok_value == K_NULL))
													{					
														col_entry[cur_id].not_null = true;
														cur = cur->next->next;
													}
		
													if (!rc)
													{
														/* I must have either a comma or right paren */
														if ((cur->tok_value != S_RIGHT_PAREN) &&															  (cur->tok_value != S_COMMA))
														{
															rc = INVALID_COLUMN_DEFINITION;
															cur->tok_value = INVALID;
														}
														else
													  {
															if (cur->tok_value == S_RIGHT_PAREN)
															{
																column_done = true;
															}
															cur = cur->next;
														}
													}
												}
											}
										}	/* end char(n) processing */
									}
								} /* end char processing */
							}
						}  // duplicate column name
					} // invalid column name

					/* If rc=0, then get ready for the next column */
					if (!rc)
					{
						cur_id++;
					}

				} while ((rc == 0) && (!column_done));
	
				if ((column_done) && (cur->tok_value != EOC))
				{
					rc = INVALID_TABLE_DEFINITION;
					cur->tok_value = INVALID;
				}

				if (!rc)
				{
					/* Now finished building tpd and add it to the tpd list */
					tab_entry.num_columns = cur_id;
					tab_entry.tpd_size = sizeof(tpd_entry) + sizeof(cd_entry) *	tab_entry.num_columns;
				  	tab_entry.cd_offset = sizeof(tpd_entry);
					new_entry = (tpd_entry*)calloc(1, tab_entry.tpd_size);

					if (new_entry == NULL)
					{
						rc = MEMORY_ERROR;
					}
					else
					{
						memcpy((void*)new_entry,
							     (void*)&tab_entry,
									 sizeof(tpd_entry));
		
						memcpy((void*)((char*)new_entry + sizeof(tpd_entry)),
									 (void*)col_entry,
									 sizeof(cd_entry) * tab_entry.num_columns);
	
						rc = initialize_tab_file(new_entry, col_entry);
						rc = add_tpd_to_list(new_entry);
						//print_col_detail(col_entry, new_entry.num_columns);

						free(new_entry);
					}
				}
			}
		}
	}
  return rc;
}

int sem_drop_table(token_list *t_list)
{
	int rc = 0;
	token_list *cur;
	tpd_entry *tab_entry = NULL;

	cur = t_list;
	if ((cur->tok_class != keyword) &&
		  (cur->tok_class != identifier) &&
			(cur->tok_class != type_name))
	{
		// Error
		rc = INVALID_TABLE_NAME;
		cur->tok_value = INVALID;
	}
	else
	{
		if (cur->next->tok_value != EOC)
		{
			rc = INVALID_STATEMENT;
			cur->next->tok_value = INVALID;
		}
		else
		{
			if ((tab_entry = get_tpd_from_list(cur->tok_string)) == NULL)
			{
				rc = TABLE_NOT_EXIST;
				cur->tok_value = INVALID;
			}
			else
			{
				/* Found a valid tpd, drop it from tpd list */
				rc = drop_tpd_from_list(cur->tok_string);
				rc = delete_tab_file(cur->tok_string);
			}
		}
	}

  return rc;
}

int sem_list_tables()
{
	int rc = 0;
	int num_tables = g_tpd_list->num_tables;
	tpd_entry *cur = &(g_tpd_list->tpd_start);

	if (num_tables == 0)
	{
		printf("\nThere are currently no tables defined\n");
	}
	else
	{
		printf("\nTable List\n");
		printf("*****************\n");
		while (num_tables-- > 0)
		{
			printf("%s\n", cur->table_name);
			if (num_tables > 0)
			{
				cur = (tpd_entry*)((char*)cur + cur->tpd_size);
			}
		}
		printf("****** End ******\n");
	}

  return rc;
}

int sem_list_schema(token_list *t_list)
{
	int rc = 0;
	token_list *cur;
	tpd_entry *tab_entry = NULL;
	cd_entry  *col_entry = NULL;
	char tab_name[MAX_IDENT_LEN+1];
	char filename[MAX_IDENT_LEN+1];
	bool report = false;
	FILE *fhandle = NULL;
	int i = 0;

	cur = t_list;

	if (cur->tok_value != K_FOR)
  {
		rc = INVALID_STATEMENT;
		cur->tok_value = INVALID;
	}
	else
	{
		cur = cur->next;

		if ((cur->tok_class != keyword) &&
			  (cur->tok_class != identifier) &&
				(cur->tok_class != type_name))
		{
			// Error
			rc = INVALID_TABLE_NAME;
			cur->tok_value = INVALID;
		}
		else
		{
			memset(filename, '\0', MAX_IDENT_LEN+1);
			strcpy(tab_name, cur->tok_string);
			cur = cur->next;

			if (cur->tok_value != EOC)
			{
				if (cur->tok_value == K_TO)
				{
					cur = cur->next;
					
					if ((cur->tok_class != keyword) &&
						  (cur->tok_class != identifier) &&
							(cur->tok_class != type_name))
					{
						// Error
						rc = INVALID_REPORT_FILE_NAME;
						cur->tok_value = INVALID;
					}
					else
					{
						if (cur->next->tok_value != EOC)
						{
							rc = INVALID_STATEMENT;
							cur->next->tok_value = INVALID;
						}
						else
						{
							/* We have a valid file name */
							strcpy(filename, cur->tok_string);
							report = true;
						}
					}
				}
				else
				{ 
					/* Missing the TO keyword */
					rc = INVALID_STATEMENT;
					cur->tok_value = INVALID;
				}
			}

			if (!rc)
			{
				if ((tab_entry = get_tpd_from_list(tab_name)) == NULL)
				{
					rc = TABLE_NOT_EXIST;
					cur->tok_value = INVALID;
				}
				else
				{
					if (report)
					{
						if((fhandle = fopen(filename, "a+tc")) == NULL)
						{
							rc = FILE_OPEN_ERROR;
						}
					}

					if (!rc)
					{
						/* Find correct tpd, need to parse column and index information */

						/* First, write the tpd_entry information */
						printf("Table PD size            (tpd_size)    = %d\n", tab_entry->tpd_size);
						printf("Table Name               (table_name)  = %s\n", tab_entry->table_name);
						printf("Number of Columns        (num_columns) = %d\n", tab_entry->num_columns);
						printf("Column Descriptor Offset (cd_offset)   = %d\n", tab_entry->cd_offset);
            printf("Table PD Flags           (tpd_flags)   = %d\n\n", tab_entry->tpd_flags); 

						if (report)
						{
							fprintf(fhandle, "Table PD size            (tpd_size)    = %d\n", tab_entry->tpd_size);
							fprintf(fhandle, "Table Name               (table_name)  = %s\n", tab_entry->table_name);
							fprintf(fhandle, "Number of Columns        (num_columns) = %d\n", tab_entry->num_columns);
							fprintf(fhandle, "Column Descriptor Offset (cd_offset)   = %d\n", tab_entry->cd_offset);
              fprintf(fhandle, "Table PD Flags           (tpd_flags)   = %d\n\n", tab_entry->tpd_flags); 
						}

						/* Next, write the cd_entry information */
						for(i = 0, col_entry = (cd_entry*)((char*)tab_entry + tab_entry->cd_offset);
								i < tab_entry->num_columns; i++, col_entry++)
						{
							printf("Column Name   (col_name) = %s\n", col_entry->col_name);
							printf("Column Id     (col_id)   = %d\n", col_entry->col_id);
							printf("Column Type   (col_type) = %d\n", col_entry->col_type);
							printf("Column Length (col_len)  = %d\n", col_entry->col_len);
							printf("Not Null flag (not_null) = %d\n\n", col_entry->not_null);

							if (report)
							{
								fprintf(fhandle, "Column Name   (col_name) = %s\n", col_entry->col_name);
								fprintf(fhandle, "Column Id     (col_id)   = %d\n", col_entry->col_id);
								fprintf(fhandle, "Column Type   (col_type) = %d\n", col_entry->col_type);
								fprintf(fhandle, "Column Length (col_len)  = %d\n", col_entry->col_len);
								fprintf(fhandle, "Not Null Flag (not_null) = %d\n\n", col_entry->not_null);
							}
						}
	
						if (report)
						{
							fflush(fhandle);
							fclose(fhandle);
						}
					} // File open error							
				} // Table not exist
			} // no semantic errors
		} // Invalid table name
	} // Invalid statement

  return rc;
}

int initialize_tpd_list()
{
	int rc = 0;
	FILE *fhandle = NULL;
	struct _stat file_stat;

  /* Open for read */
  if((fhandle = fopen("dbfile.bin", "rbc")) == NULL)
	{
		if((fhandle = fopen("dbfile.bin", "wbc")) == NULL)
		{
			rc = FILE_OPEN_ERROR;
		}
    else
		{
			g_tpd_list = NULL;
			g_tpd_list = (tpd_list*)calloc(1, sizeof(tpd_list));
			
			if (!g_tpd_list)
			{
				rc = MEMORY_ERROR;
			}
			else
			{
				g_tpd_list->list_size = sizeof(tpd_list);
				fwrite(g_tpd_list, sizeof(tpd_list), 1, fhandle);
				fflush(fhandle);
				fclose(fhandle);
			}
		}
	}
	else
	{
		/* There is a valid dbfile.bin file - get file size */
		_fstat(_fileno(fhandle), &file_stat);
		printf("dbfile.bin size = %d\n", file_stat.st_size);

		g_tpd_list = (tpd_list*)calloc(1, file_stat.st_size);

		if (!g_tpd_list)
		{
			rc = MEMORY_ERROR;
		}
		else
		{
			fread(g_tpd_list, file_stat.st_size, 1, fhandle);
			fflush(fhandle);
			fclose(fhandle);

			if (g_tpd_list->list_size != file_stat.st_size)
			{
				rc = DBFILE_CORRUPTION;
			}

		}
	}
    
	return rc;
}
	
int add_tpd_to_list(tpd_entry *tpd)
{
	int rc = 0;
	int old_size = 0;
	FILE *fhandle = NULL;

	if((fhandle = fopen("dbfile.bin", "wbc")) == NULL)
	{
		rc = FILE_OPEN_ERROR;
	}
  	else
	{
		old_size = g_tpd_list->list_size;
		// printf("ADDING tpd to list\n");
		// printf("BEFORE g_tpd_list->list_size :%d if 0 table: %d old_size: %d fwrite size: %d\n", g_tpd_list->list_size, tpd->tpd_size - sizeof(tpd_entry), old_size, old_size - sizeof(tpd_entry));

		if (g_tpd_list->num_tables == 0)
		{
			/* If this is an empty list, overlap the dummy header */
			g_tpd_list->num_tables++;
		 	g_tpd_list->list_size += (tpd->tpd_size - sizeof(tpd_entry));
			fwrite(g_tpd_list, old_size - sizeof(tpd_entry), 1, fhandle);
		}
		else
		{
			/* There is at least 1, just append at the end */
			g_tpd_list->num_tables++;
		 	g_tpd_list->list_size += tpd->tpd_size;
			fwrite(g_tpd_list, old_size, 1, fhandle);
		}
		// printf("AFTER tpd to list----\n");
		// printf("AFTER g_tpd_list->list_size :%d if 0 table: %d tpd_size: %d fwrite size: %d\n", g_tpd_list->list_size, tpd->tpd_size - sizeof(tpd_entry), tpd->tpd_size, old_size);

		fwrite(tpd, tpd->tpd_size, 1, fhandle);
		fflush(fhandle);
		fclose(fhandle);
	}

	return rc;
}

int drop_tpd_from_list(char *tabname)
{
	int rc = 0;
	tpd_entry *cur = &(g_tpd_list->tpd_start);
	int num_tables = g_tpd_list->num_tables;
	bool found = false;
	int count = 0;

	if (num_tables > 0)
	{
		while ((!found) && (num_tables-- > 0))
		{
			if (stricmp(cur->table_name, tabname) == 0)
			{
				/* found it */
				found = true;
				int old_size = 0;
				FILE *fhandle = NULL;

				if((fhandle = fopen("dbfile.bin", "wbc")) == NULL)
				{
					rc = FILE_OPEN_ERROR;
				}
			  else
				{
					old_size = g_tpd_list->list_size;

					if (count == 0)
					{
						/* If this is the first entry */
						g_tpd_list->num_tables--;

						if (g_tpd_list->num_tables == 0)
						{
							/* This is the last table, null out dummy header */
							memset((void*)g_tpd_list, '\0', sizeof(tpd_list));
							g_tpd_list->list_size = sizeof(tpd_list);
							fwrite(g_tpd_list, sizeof(tpd_list), 1, fhandle);
						}
						else
						{
							/* First in list, but not the last one */
							g_tpd_list->list_size -= cur->tpd_size;

							/* First, write the 8 byte header */
							fwrite(g_tpd_list, sizeof(tpd_list) - sizeof(tpd_entry),
								     1, fhandle);

							/* Now write everything starting after the cur entry */
							fwrite((char*)cur + cur->tpd_size,
								     old_size - cur->tpd_size -
										 (sizeof(tpd_list) - sizeof(tpd_entry)),
								     1, fhandle);
						}
					}
					else
					{
						/* This is NOT the first entry - count > 0 */
						g_tpd_list->num_tables--;
					 	g_tpd_list->list_size -= cur->tpd_size;

						/* First, write everything from beginning to cur */
						fwrite(g_tpd_list, ((char*)cur - (char*)g_tpd_list),
									 1, fhandle);

						/* Check if cur is the last entry. Note that g_tdp_list->list_size
						   has already subtracted the cur->tpd_size, therefore it will
						   point to the start of cur if cur was the last entry */
						if ((char*)g_tpd_list + g_tpd_list->list_size == (char*)cur)
						{
							/* If true, nothing else to write */
						}
						else
						{
							/* NOT the last entry, copy everything from the beginning of the
							   next entry which is (cur + cur->tpd_size) and the remaining size */
							fwrite((char*)cur + cur->tpd_size,
										 old_size - cur->tpd_size -
										 ((char*)cur - (char*)g_tpd_list),							     
								     1, fhandle);
						}
					}

					fflush(fhandle);
					fclose(fhandle);
				}

				
			}
			else
			{
				if (num_tables > 0)
				{
					cur = (tpd_entry*)((char*)cur + cur->tpd_size);
					count++;
				}
			}
		}
	}
	
	if (!found)
	{
		rc = INVALID_TABLE_NAME;
	}

	return rc;
}

tpd_entry* get_tpd_from_list(char *tabname)
{
	tpd_entry *tpd = NULL;
	tpd_entry *cur = &(g_tpd_list->tpd_start);
	int num_tables = g_tpd_list->num_tables;
	bool found = false;

	if (num_tables > 0)
	{
		while ((!found) && (num_tables-- > 0))
		{
			if (stricmp(cur->table_name, tabname) == 0)
			{
				/* found it */
				found = true;
				tpd = cur;
			}
			else
			{
				if (num_tables > 0)
				{
					cur = (tpd_entry*)((char*)cur + cur->tpd_size);
				}
			}
		}
	}

	return tpd;
}

/*
new functions additional to the original codes
*/


int cal_record_size_table_header (cd_entry *col_entry, int num){
	int i = 0, total_bytes = 0;
	while (i < num){
		// printf("%s\n", col_entry[i].col_name );
		if (col_entry[i].col_type==T_CHAR)
		{
			// printf("#%d len is %d\n", i, col_entry[i].col_len);
			total_bytes+= 1+col_entry[i].col_len;
		}
		else{
			total_bytes+=1+4;
		}
		i++;
	}
	if (total_bytes %4 != 0)
	{
		total_bytes = total_bytes + (4- total_bytes% 4);
	}

	// printf("Total Bytes are: %d\n", total_bytes);
	return total_bytes;
}

int delete_tab_file(char *table_name){
	int rc = 0, err=0;
	char entry_name[MAX_IDENT_LEN+4+4];

	strcpy(entry_name, table_name);
	// strcat(entry_name, ".bin");
	strcat(entry_name, ".tab");

	// printf("%s detected\n", entry_name);
	rc = remove(entry_name);

	return rc;
}

int initialize_tab_file(tpd_entry *tpd, cd_entry *col_entry)
{
	int rc = 0;
	FILE *fhandle = NULL;
	struct _stat file_stat;
	char entry_name[MAX_IDENT_LEN+4+4];

	// table_file_header *tf_header = (table_file_header*)malloc(sizeof(tf_header));
	// memset(tf_header, '\0', sizeof(tf_header));

	// printf("memset g_tf_header\n");
	// g_tf_header = (table_file_header*)malloc(sizeof(table_file_header));
	// memset(g_tf_header, '\0', sizeof(table_file_header));

	strcpy(entry_name, tpd->table_name);
	// strcat(entry_name, ".bin");
	strcat(entry_name, ".tab");


  /* Open for read */
	// printf("reached if \n");
  if((fhandle = fopen(entry_name, "rbc")) == NULL)
	{
		if((fhandle = fopen(entry_name, "wbc")) == NULL)
		{
			rc = FILE_OPEN_ERROR;
		}
    else
		{
			table_file_header* file_header = NULL;
			file_header = (table_file_header*)calloc(1, sizeof(table_file_header));
			memset(file_header, '\0', sizeof(table_file_header));
			
			if (!file_header)
			{
				rc = MEMORY_ERROR;
			}
			else
			{
				// printf("reached if \n");

				//g_tpd_list->list_size = sizeof(tpd_list);
				file_header->record_size = cal_record_size_table_header(col_entry, tpd->num_columns);
				file_header->file_size = sizeof(table_file_header);
				file_header->num_records = 0;
				file_header->record_offset = sizeof(table_file_header);
				file_header->tpd_ptr = tpd;
				// printf("REACHED MEMCPY\n");
				fwrite(file_header, sizeof(table_file_header), 1, fhandle);
				fflush(fhandle);
				fclose(fhandle);
				// printf("finished fclose\n");
				// tests
				// printf("file_header\nfile_size: %d\t record_size: %d\t num_records: %d\t record_offset: %d \t \n", file_header->file_size, file_header->record_size, file_header->num_records, file_header->record_offset);
				// printf("file_header_tpd_entry size is: %d\n (table name: %s) (column number: %d) (offset: %d)\n\n", file_header->tpd_ptr->tpd_size, file_header->tpd_ptr->table_name, file_header->tpd_ptr->num_columns, file_header->tpd_ptr->cd_offset);

			}

			free(file_header);
		}
	}
	else
	{
		/* There is a valid dbfile.bin file - get file size */

	}
	return rc;
}

int sem_insert(token_list *t_list)
{
	int rc = 0, record_size =0, offset =0;
	token_list *cur, *o_cur, *r_paren = NULL;
	int cur_id = 0, do_while_control= 0;
	tpd_entry *tab_entry;
	cd_entry *col_entry;
	

	cur = t_list;

	int zero_one[3];
	zero_one[0] = 0;
	zero_one[1] = 4;
	zero_one[2] = 0;
	int temp_int = 0;
	// printf("token is %s\n", cur->tok_string);

	// if the token string is not a identifier
	if (cur->tok_class != identifier)
	{
		// Error
		// printf("token class is %d\n", cur->tok_class);
		rc = INVALID_TABLE_NAME;
		cur->tok_value = INVALID;
	}
	// of if the table does not exist
	else if ((tab_entry = get_tpd_from_list(cur->tok_string)) == NULL)
	{
		rc = TABLE_NOT_EXIST;
		cur->tok_value = INVALID;
	}

	else{

		// printf("INSERT else token is %s\n tab_entry: (table name: %s) (column number: %d) (offset: %d)\n", cur->tok_string, tab_entry->table_name, tab_entry->num_columns, tab_entry->cd_offset);
		cur = cur->next;
		if (cur->tok_value != K_VALUES)
		{
			printf("values keyword missing");
			rc = INVALID_STATEMENT;
			cur->tok_value = INVALID;
		}
		else{
			cur = cur->next;
			// check if the cur is the left (
			if (cur->tok_value != S_LEFT_PAREN)
			{
				rc = INVALID_STATEMENT;
				cur->tok_value = INVALID;
			}
			else{
				o_cur = cur->next;
				cur = cur->next;
				while(cur->tok_value!=EOC){
					if (cur->tok_value == S_RIGHT_PAREN)
					{
						r_paren = cur;
					}
					cur = cur->next;
				}

				if (r_paren == NULL)
				{
					rc = INVALID_STATEMENT;
					cur->tok_value = INVALID;
					// printf("NO ))))\n");
				} // no right ) is found
				else{
					cur = o_cur;
					
					do{
						// printf("SO FAR SO GOOD! Current cur is %s\n", cur->tok_string);
						col_entry = (cd_entry*)((char*)tab_entry + tab_entry->cd_offset*(1+do_while_control));

						// printf("Round: %d\n", do_while_control);
						// printf("Column Name   (col_name) = %s\n", col_entry->col_name);
						// printf("Column Id     (col_id)   = %d\n", col_entry->col_id);
						// printf("Column Type   (col_type) = %d\n", col_entry->col_type);
						// printf("Column Length (col_len)  = %d\n", col_entry->col_len);
						// printf("Not Null flag (not_null) = %d\n\n", col_entry->not_null);

						if (cur->tok_value == K_NULL)
						{
							if (col_entry->not_null == 1)
							{
								printf("Not Null constraint exists for column name %s\n", col_entry->col_name);
								rc = NOT_NULL_CONSTR;
								cur->tok_value = INVALID;
								break;
							}
							else{

							}
						}

						if (cur->tok_value == INT_LITERAL)
						{
							// printf("INT is detected in PROCESSING ARRGUMENT\n");
							// if the corresponding column type is not int, the return rc = ILLEGAL_ARGUMENT_TYPE
							if (col_entry->col_type != T_INT)
							{
								printf("Type mismatch\n");
								rc = MISMATCH_TYPE;
								cur->tok_value = INVALID;
								break;
							}
							
						}

						if (cur->tok_value == STRING_LITERAL)
						{
							// printf("STRING is detected in PROCESSING ARRGUMENT\n");
							if (col_entry->col_type != T_CHAR)
							{
								printf("Type mismatch\n");
								rc = MISMATCH_TYPE;
								cur->tok_value = INVALID;
								break;
							}
							else{
								// printf("PROCESSING CHAR:: column length: %d, token length for %s is %d\n", col_entry->col_len, cur->tok_string, strlen(cur->tok_string));
								
								if (col_entry->col_len < strlen(cur->tok_string))
								{
									rc = MAX_LENGTH_EXCEEDED;
									cur->tok_value = INVALID;
									break;
								}
							}
						}

						cur = cur->next;
						// DETECTing a comma. If detected a comma, cur will move to the next; if a ), will test condition, else, illegal
						if (cur->tok_class != symbol )
						{
							// printf("COMMA ))) issue, tok is %d\n", cur->tok_value);
							printf("Missing comma, possible that number of columns and insert values don't match\n");
							rc = MISSING_COMMA_OR_NOT_ENOUGH_LEN;
							cur->tok_value = INVALID;
							break;
						}
						else{
							// or reach the end right paran

							if (cur->tok_value == S_COMMA){

							}

							else{
								if (cur->tok_value == S_RIGHT_PAREN)
								{
									if (do_while_control != (tab_entry->num_columns -1))
									{
										printf("Missing comma, possible that number of columns and insert values don't match\n");
										rc = MISSING_COMMA_OR_NOT_ENOUGH_LEN;
										cur->tok_value = INVALID;
										break;
									}
								}
							}
							cur = cur->next;
						}
						do_while_control++;
					}while(do_while_control<tab_entry->num_columns);
					// only goes through num_columns times, extra will produce error

					// printf("END DO WHILE LOOP, tok_value is: %d\n", cur->tok_value);
					if (cur->tok_value != EOC)
					{
						rc = UNSUCESSFUL_INSERT;
						cur->tok_value = INVALID;
					}

					col_entry = (cd_entry*)((char*)tab_entry + tab_entry->cd_offset);
					record_size = cal_record_size_table_header(col_entry, tab_entry->num_columns);
					// printf("Return code is: %d, record_size is: %d\n", rc, record_size);



				}

			}
		}// if the next token is not values
	}

	// finished checking syntax in the statement.
	o_cur = t_list;
	o_cur = o_cur->next;
	o_cur = o_cur->next;

	// printf("o_cur tok = %s\n", o_cur->tok_string);
	do_while_control = 0;
	insert_row* r = (insert_row*)calloc(1, record_size);
	do{
		o_cur = o_cur->next;

		// typedef struct t_list
		// {
		// 	char	tok_string[MAX_TOK_LEN];
		// 	int		tok_class;
		// 	int		tok_value;
		// 	struct t_list *next;
		// } token_list;
		// printf("o_cur: string(%s), strlen(string: %d), class(%d), value(%d)\n", o_cur->tok_string, strlen(o_cur->tok_string), o_cur->tok_class, o_cur->tok_value);
		
		// ptr = (token_list*)calloc(1, sizeof(token_list));

		col_entry = (cd_entry*)((char*)tab_entry + tab_entry->cd_offset*(1+do_while_control));

		if (o_cur->tok_value == K_NULL)
		{
			if (col_entry->col_type == T_INT)
			{
				memcpy(r+offset, &zero_one[0], 1);
				offset+=1;
				memcpy(r+offset, &zero_one[0], sizeof(int));
				offset+=4;
			}
			
			//?????
			if (col_entry->col_type == T_CHAR){
				memcpy(r+offset, &zero_one[0], 1);
				offset+=1;
				memcpy(r+offset, &zero_one[0], col_entry->col_len);
				offset+=col_entry->col_len;
			}

		}

		if (o_cur->tok_value == INT_LITERAL)
		{
			memcpy(r+offset, &zero_one[1], 1);
			offset+=1;
			temp_int = atoi(o_cur->tok_string);
			memcpy(r+offset, &temp_int, sizeof(int));
			offset+=4;
		}

		if (o_cur->tok_value == STRING_LITERAL || o_cur->tok_value == IDENT )
		{
			// printf("ocur-tok_string: %s, col_entry-> col_len = %d\n token len is %d",o_cur->tok_string, col_entry->col_len, strlen(o_cur->tok_string));
			zero_one[2] = strlen(o_cur->tok_string);
			memcpy(r+offset, &zero_one[2], 1);
			offset+=1;
			memcpy(r+offset, o_cur->tok_string, col_entry->col_len);
			offset+=col_entry->col_len;
		}

		o_cur=o_cur->next;
		do_while_control++;

	}while(do_while_control<tab_entry->num_columns);
	// printf("\n\nEND of INSERT offset: %d rc is: %d\n ", offset, rc);
	// print_bytes(r, record_size);

	if (rc == 0)
	{
		// printf("Query OK. 1 row effected\n");;
		rc = update_table_file(tab_entry->table_name, r, record_size);
	}

	return rc;
}

void print_bytes(const void *object, size_t size)
{
  size_t i;

  printf("[ ");
  for(i = 0; i < size; i++)
  {
    printf("%02x ", ((const unsigned char *) object)[i] & 0xff);
  }
  printf("]\n");
}

int update_table_file(char *table_name, insert_row *row, int row_size){
	int rc = 0;
	FILE *fhandle = NULL;
	char fname[MAX_IDENT_LEN + 4];
	void *buffer;
	table_file_header *file_header;

	file_header = NULL;

	// printf("DEBUGGING update_table_file %s\n", table_name);

	memset(fname, '\0', MAX_IDENT_LEN + 4);
	strcat(fname, table_name);
	// strcat(fname, ".bin");
	strcat(fname, ".tab");


	if ((fhandle = fopen(fname, "a+tc")) == NULL)
	{
		rc = FILE_OPEN_ERROR;
	}
	else
	{
		// print_bytes(row, row_size);
		fwrite(row, row_size, 1, fhandle);
	}

	//fflush(fhandle);
	fclose(fhandle);

	if ((fhandle = fopen(fname, "rbc")) == NULL)
	{
		rc = FILE_OPEN_ERROR;
	}
	else
	{
		buffer = malloc(sizeof(table_file_header));
		fread(buffer, sizeof(table_file_header), 1, fhandle);
		// printf("PRINTING BUFFER-table file header in bytes\n");
		// print_bytes(buffer, sizeof(table_file_header));
		file_header = (table_file_header*) buffer;
		// tests
		// printf("READING FILE\n\nfile_header\nfile_size: %d\t record_size: %d\t num_records: %d\t record_offset: %d \t \n", file_header->file_size, file_header->record_size, file_header->num_records, file_header->record_offset);
	}

	fclose(fhandle);

	// over write the table file header with updated file size and num of records
	if ((fhandle = fopen(fname, "r+b")) == NULL)
	{
		rc = FILE_OPEN_ERROR;
	}
	else
	{
		file_header->file_size += file_header->record_size;
		file_header->num_records++;

		fwrite(file_header, sizeof(table_file_header), 1, fhandle);

	}

	fclose(fhandle);

	// check if the updated table file header is as expected.
	if ((fhandle = fopen(fname, "rbc")) == NULL)
	{
		rc = FILE_OPEN_ERROR;
	}
	else
	{
		buffer = malloc(sizeof(table_file_header));
		fread(buffer, sizeof(table_file_header), 1, fhandle);
		// print_bytes(buffer, sizeof(table_file_header));
		file_header = (table_file_header*) buffer;
		// tests
		printf("%s.tab size: %d. Number of records: %d\n", table_name, file_header->file_size, file_header->num_records);
		// printf("READING FILE\n\nfile_header\nfile_size: %d\t record_size: %d\t num_records: %d\t record_offset: %d \t \n", file_header->file_size, file_header->record_size, file_header->num_records, file_header->record_offset);
	}

	fclose(fhandle);


	return rc;
}


int sem_select(token_list *t_list)
{
	int rc = 0, record_size =0, offset =0;
	token_list *cur, *o_cur;
	int cur_id = 0, do_while_control= 0, do_while_control_2 = 0;
	int print_header = 1, temp_row_offset = 0;
	int zero_one[2];
	tpd_entry *tab_entry;
	cd_entry *col_entry;

	unsigned char* null_byte = NULL;
	int temp, temp_int_value;
	unsigned char temp_null = 0;
	null_byte = &temp_null;
	memset(&temp, '\0', sizeof(int));
	memset(&temp_int_value, '\0', sizeof(int));

	char* temp_string = NULL;
	temp_string = (char *)calloc(1, MAX_TOK_LEN);

	FILE *fhandle = NULL;
	char fname[MAX_IDENT_LEN + 4];
	void *buffer, *new_buffer;
	table_file_header *file_header;

	file_header = NULL;
	insert_row* r = NULL;
	insert_row* temp_row = NULL;

	cur = t_list;
	zero_one[0] = 0;
	zero_one[1] = 1;
	// test if the cur is * or not.
	if (cur->tok_value != S_STAR)
	{
		rc = INVALID_STATEMENT;
		cur->tok_value = INVALID;
	}
	else{
		cur = cur->next;

		//find keyword from
		if (cur->tok_value != K_FROM)
		{
			rc = INVALID_STATEMENT;
			cur->tok_value = INVALID;
		}
		else{
			cur = cur->next;

			// test if the cur is the table identifier
			if (cur->tok_class != identifier)
			{
				rc = INVALID_STATEMENT;
				cur->tok_value = INVALID;
			}
			else{
				cur = cur->next;
				// test if end of file is reached
				if (cur->tok_class != terminator)
				{
					rc = INVALID_STATEMENT;
					cur->tok_value = INVALID;
				}
				else{
					// printf("DEBUGGING SELECT * FROM TABLE -- SUCCESS\n");
				}
			}
		}
	}

	o_cur = t_list;
	o_cur = o_cur->next->next;

	if ((tab_entry = get_tpd_from_list(o_cur->tok_string)) == NULL)
	{
		rc = TABLE_NOT_EXIST;
		o_cur->tok_value = INVALID;
		return rc;
	}
	else{
		memset(fname, '\0', MAX_IDENT_LEN + 4);
		strcat(fname, o_cur->tok_string);
		// strcat(fname, ".bin");
		 strcat(fname, ".tab");

	}

	// printf("TABLE FOUND: %s\n", fname);
	// printf("return code is %d\n", rc);

	if ((fhandle = fopen(fname, "rbc")) == NULL)
	{
		rc = FILE_OPEN_ERROR;
		return rc;
		
	}
	else
	{
		// read table file header from file
		buffer = malloc(sizeof(table_file_header));
		fread(buffer, sizeof(table_file_header), 1, fhandle);
		// print_bytes(buffer, sizeof(table_file_header));
		// printf("**Select PRINTING BUFFER-table file header in bytes\n");
		// print_bytes(buffer, sizeof(table_file_header));
		file_header = (table_file_header*) buffer;
		// tests
		// printf("READING FILE\n\nfile_header\nfile_size: %d\t record_size: %d\t num_records: %d\t record_offset: %d \t \n", file_header->file_size, file_header->record_size, file_header->num_records, file_header->record_offset);
	}

	fclose(fhandle);

	new_buffer = calloc(1, file_header->file_size);

	if ((fhandle = fopen(fname, "rb")) == NULL)
	{
		rc = FILE_OPEN_ERROR;
		return rc;
	}
	else
	{
		// copy whole file to memory
		memset(new_buffer, '\0', file_header->file_size);
		fread(new_buffer, file_header->file_size, 1, fhandle);
		// print_bytes(new_buffer, file_header->file_size);
	}
	fclose(fhandle);
	//looks good till here
	// printing columns
	r = (insert_row*)calloc(1, file_header->file_size - file_header->record_offset);
	memset(r, '\0', file_header->file_size - file_header->record_offset);
	temp_row = (insert_row*)calloc(1, file_header->record_size);
	memset(temp_row, '\0', file_header->record_size);
	offset = file_header->record_offset;

	//(*(char *)&voidptr) += 6 
	// printf("Printing bytes for data section offset: %d\n", offset);
	memcpy(r, ((char *)new_buffer)+offset, file_header->file_size - file_header->record_offset);
	// print_bytes(r, file_header->file_size - offset);
	do_while_control=0;
	offset = 1;
	do_while_control_2 = 0;
	// start to print separaters
	char line_char[(1+MAX_IDENT_LEN)*MAX_NUM_COL];
	int line_char_i = 0;
	for (line_char_i = 0; line_char_i < (1+MAX_IDENT_LEN)*MAX_NUM_COL; ++line_char_i)
	{
		line_char[0] = '\0';
	}

	int print_line_loop;
	for (print_line_loop = 0; print_line_loop < tab_entry->num_columns; ++print_line_loop)
	{
		strcat(line_char, "+----------------");
	}
	printf("%s+\n", line_char);

	do{
		int num_place_holder, print_i = 0;
		col_entry = (cd_entry*)((char*)tab_entry + tab_entry->cd_offset*(1+do_while_control_2));
		num_place_holder = MAX_IDENT_LEN - strlen(col_entry->col_name);
		printf("|%s", col_entry->col_name);
		for(print_i = 0; print_i < num_place_holder; print_i++){
			printf(" ");
		}
		do_while_control_2 +=1;

	}while(do_while_control_2 < tab_entry->num_columns);
	printf("|\n");
	printf("%s+\n", line_char);


	do{
		memcpy(temp_row, r+offset-1, file_header->record_size);
		offset += file_header->record_size;
		// printf("PRINTING DATA RECORD FROM TABLE FILE offset:%d\n", offset);
		// print_bytes(temp_row, file_header->record_size);
		// printf("\t\tNew DATA\n");
		do_while_control_2 = 0;
		temp_row_offset = 0;
		do{
			// printf("\tStart looping inside of data with offset: %d\n", temp_row_offset);

			col_entry = (cd_entry*)((char*)tab_entry + tab_entry->cd_offset*(1+do_while_control_2));

			// printf("Column Name   (col_name) = %s\n", col_entry->col_name);
			// printf("Column Id     (col_id)   = %d\n", col_entry->col_id);
			// printf("Column Type   (col_type) = %d\n", col_entry->col_type);
			// printf("Column Length (col_len)  = %d\n", col_entry->col_len);
			// printf("Not Null flag (not_null) = %d\n\n", col_entry->not_null);

			// if the collumn type is int
			if (col_entry->col_type == T_INT)
			{
				// printf("Checking int\n");
				// store the null_byte from the table.tab file, then increment by 1
				memcpy(null_byte, temp_row+temp_row_offset, 1);
				temp_row_offset+=1;
				temp =  *null_byte;
				// printf("int_null_byte is : %d\n", temp);
				
				// the input is not null.
				if (temp > 0)
				{
					int temp_digit = 0, temp_num = 0;
					memcpy(&temp_int_value, temp_row+temp_row_offset, sizeof(int));
					// printf("Converted number is : %d\n", temp_int_value);
					// temp_mun and do while is to check how many digits is the int
					temp_num = temp_int_value;
					do{
						temp_num /= 10; 
						temp_digit++;
					}
					while(temp_num != 0);
					
					// if the digit is 0, the digit of 0 should be 1 instead of 0
					if(temp_int_value == 0){
						temp_digit = 1;
					}
					
					// print the place holder
					printf("|%*c", MAX_IDENT_LEN-temp_digit, ' ');
					printf("%d", temp_int_value);
					// printf("|   %d\t\t", temp_int_value);

				}
				else
				{
					printf("NULL            ");
				}

				temp_row_offset+=4;


			}else{
				// printf("Checking char\n");
				memcpy(null_byte, temp_row+temp_row_offset, 1);
				temp_row_offset+=1;
				temp =  *null_byte;				
				// printf("char_null_byte is : %d\n", temp);

				if (temp > 0)
				{
					// printf("char size is: %d\n", col_entry->col_len);
					// store the string bytes to temp_string variable
					memset (temp_string, '\0', sizeof(col_entry->col_len));
					memcpy(temp_string, temp_row+temp_row_offset, col_entry->col_len);
					
					// print place hoders
					if (col_entry->col_len == 1)
					{
						// printf("Converted string is : %c\n", temp_string[0]);
						printf("|%c", temp_string[0]);
						printf("%*c", 15, ' ');

					}
					else{
						// printf("Converted string is : %s\n", temp_string);
						printf("|%s", temp_string);
						printf("%*c", MAX_IDENT_LEN-strlen(temp_string), ' ');

					}
				}
				else
				{
					printf("|NULL            ");
				}
				temp_row_offset+= col_entry->col_len;
			}

			do_while_control_2+=1;
		}while(do_while_control_2 < tab_entry->num_columns);
		printf("|\n");

		// o_cur=o_cur->next;
		do_while_control++;

	}while(do_while_control<file_header->num_records);
	printf("%s+\n", line_char);
	// printf("Reached the end of select\n");
	printf("%d rows selected.\n", file_header->num_records);

	return rc;
}