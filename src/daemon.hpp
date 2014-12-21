#pragma once

#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>

#include <microhttpd.h>

class Daemon
{
public:
	
	static const int YES = MHD_YES;
	static const int NO  = MHD_NO;
	
private:

	struct PostProcessor
	{
		void *data;
		int size;
	};
	
	static int request_received(
		void *cls, struct MHD_Connection *connection, 
		const char *url, 
		const char *method, const char *version, 
		const char *upload_data, 
		size_t *upload_data_size, void **con_cls
	)
	{
		Daemon *object = static_cast<Daemon*>(cls);
		
		printf("New %s request\n",method);
    
    if(!strcmp(method,"POST"))
    {
			if(*con_cls == nullptr)
			{
				PostProcessor *pp = new PostProcessor;
				pp->data = nullptr;
				pp->size = 0;
				
				printf("Post processor created\n");
				
				*con_cls = static_cast<void*>(pp);
				return MHD_YES;
			}
			
			if(*upload_data_size != 0)
			{
				PostProcessor *pp = static_cast<PostProcessor*>(*con_cls);
				
				printf("Post iteration\n");
				
				const char *data = upload_data;
				int size = *upload_data_size;
				
				if(!pp->data)
				{
					pp->data = static_cast<void*>(new char[size]);
					pp->size = size;
					memcpy(pp->data,static_cast<const void*>(data),size);
				}
				else
				{
					void *ndata = static_cast<void*>(new char[pp->size + size]);
					memcpy(ndata,pp->data,pp->size);
					delete static_cast<char*>(pp->data);
					memcpy(
						static_cast<void*>(static_cast<char*>(ndata) + pp->size),
						static_cast<const void*>(data),
						size
					);
					pp->data = ndata;
					pp->size += size;
				}
				
				printf("Uploaded data size: %lu\n",*upload_data_size);
				*upload_data_size = 0;
				
				return MHD_YES;
			}
			else
			{
				printf("Sending post back\n");
				PostProcessor *pp = static_cast<PostProcessor*>(*con_cls);
				printf("Total data size: %d\n",pp->size);
				return object->respondPost(connection,url,pp->data,pp->size);
			}
		}
		else
		if(!strcmp(method,"GET"))
		{
			*con_cls = nullptr;
			return object->respondGet(connection,url);
		}
		return MHD_NO;
	}
	
	static int request_completed(
		void *cls, struct MHD_Connection *connection, 
		void **con_cls, enum MHD_RequestTerminationCode toe
	)
	{
		printf("Request completed\n");
		if(!con_cls)
		{
			PostProcessor *pp = static_cast<PostProcessor*>(*con_cls);
			delete static_cast<char*>(pp->data);
			delete pp;
		}
		return MHD_YES;
	}
	
	/* data is heap-allocated and you must free it */
	static void load_file(const char *path, void **data, int *len)
	{
		FILE *file = fopen(path,"r");
		if(file)
		{
			char *string;
			int size;
			
			fseek(file, 0L, SEEK_END);
			size = ftell(file);
			fseek(file, 0L, SEEK_SET);
			
			string = new char[size + 1];
			fread(string, size, 1, file);
			fclose(file);

			string[size] = '\0';
			
			*data = static_cast<void*>(string);
			*len = size;
		}
		else
		{
			*data = nullptr;
			*len = 0;
		}
	}

private:
	unsigned int _port;
	struct MHD_Daemon *_daemon;

public:
	Daemon(unsigned int port = 8888) :
		_port(port), 
		_daemon(
		  MHD_start_daemon(
		    MHD_USE_SELECT_INTERNALLY, _port, NULL, NULL, 
		    &request_received, static_cast<void*>(this), 
		    MHD_OPTION_NOTIFY_COMPLETED, &request_completed, static_cast<void*>(this), 
		    MHD_OPTION_END
		  )
		)
	{
	
	}
	
	virtual ~Daemon()
	{
		MHD_stop_daemon (_daemon);
	}
	
	/* Sends heap-allocateted data
	 * Memory will automatically released after sending
	 * free() must not be called manually */
	int sendData(MHD_Connection *con, void *data, int size)
	{
		MHD_Response *response = MHD_create_response_from_buffer(size, data, MHD_RESPMEM_MUST_FREE);
		int ret = MHD_queue_response(con, MHD_HTTP_OK, response);
		MHD_destroy_response(response);
		return ret;
	}
	
	/* Sends file content */
	int sendFile(MHD_Connection *con, const char *name)
	{
		void *page_data = nullptr;
		int page_length = 0;
		
		load_file(name,&page_data,&page_length);
		
		MHD_Response *response = nullptr;
		if(page_data)
		{
			response = MHD_create_response_from_buffer (page_length, page_data, MHD_RESPMEM_MUST_FREE);
			int ret = MHD_queue_response (con, MHD_HTTP_OK, response);
			MHD_destroy_response (response);
			return ret;
		}
		else
		{
			load_file("res/notfound.html",&page_data,&page_length);
			response = MHD_create_response_from_buffer (page_length, page_data, MHD_RESPMEM_MUST_FREE);
			MHD_queue_response (con, 404, response);
			MHD_destroy_response(response);
			return MHD_YES;
		}
	}
	
	virtual int respondGet(MHD_Connection *con, const char *url) = 0;
	virtual int respondPost(MHD_Connection *con, const char *url, void *data, int size) = 0;
};
