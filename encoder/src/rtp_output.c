/*****************************************************************************
 * raw.c: raw muxer
 *****************************************************************************
 * Copyright (C) 2003-2010 x264 project
 *
 * Authors: Laurent Aimar <fenrir@via.ecp.fr>
 *          Loren Merritt <lorenm@u.washington.edu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111, USA.
 *
 * This program is also available under a commercial proprietary license.
 * For more information, contact us at licensing@x264.com.
 *****************************************************************************/

//#include "encoder.h"
#include "output.h"
#include "socket.h"

/*
typedef struct 
{
	FILE* log;
        FILE* binaryOut;
	//SOCKET socket;
	int  socket;
	
	unsigned long timestamp;
	short sequenznummer;
	int ssrc;

        x264_nal_t* pps;
        x264_nal_t* sps;

	
} rtp_out_handle;
*/

//tools
//! \brief gibt die x264 Parameter aus
void print_x264_param(x264_param_t* p_param)
{
    if(!p_param) return;

    printf("--------------- x264 Parameter ------------------\n");

    printf("CPU-Flags: \n\n");
    printf("CPU: %d\n", p_param->cpu);
    printf("anzahl Threads: %d\n", p_param->i_threads);
    printf("sliced base Threading: %d\n", p_param->b_sliced_threads);
    printf("allowed non-deterministic optimizations when threading: %d\n", p_param->b_deterministic);
    printf("theaded lookahead buffer: %d\n", p_param->i_sync_lookahead);

    printf("\nVideo Eigenschaften:\n\n");
    printf("Breite: %d\n", p_param->i_width);
    printf("Hoehe: %d\n", p_param->i_height);
    printf("CSP ofencoded bitstream,only i420 supported: %d\n", p_param->i_csp);
    printf("level idc: %d\n", p_param->i_level_idc);
    printf("Anzahl gesamt Frame zum encoden, wenn bekannt, sonst 0: %d\n", p_param->i_frame_total);

    printf("\n------------------ Ende ----------------\n\n");

}

const char g_slice_types[] = {'a', 'I', 'i', 'p', 'B', 'b', 'k'};

//void print_x264_picture(x264_picture_t* picture)

//hnd_t ist ein void*
//! \brief oeffne den Stream
//! \param psz_target host-adresse gefolgt von : und Portnummer
//! \param p_handle zeiger auf den socket
//! \return 0 wenn alles okay, bei Fehler -1
static short sequenznummer;
static int open_file( char *psz_target, hnd_t *p_handle, cli_output_opt_t *opt)
{
    static int loop;
    static int ssrc;
    char server[] = "localhost";
    char cport[20];
    printf("oeffne Stream (open_file: %s)\n", psz_target);
	// get port from string
	//char port[32];
    //char server[256]; memset(server, 0, 256);
    //strcpy(server, get_serverAddress(psz_target));
    
	
//	strcpy(port, get_filename_port(server));
	//const char* port_str = get_filename_port(server);
	//int port = atoi(port_str);
	// delete port from string
//	char* point = server + strlen(server)-(strlen(port)+1);
//	memset(point, 0, sizeof(char*)*strlen(port));
        //port = (rand()% 265) + 40844;
    port = 40844;
        sprintf(cport, "%d", port);
	
	// debug info
//#ifdef __DEBUG
	printf("adress: %s, port: %s\n", server, cport);	
//#endif
	
	//*p_handle = fopen("log.txt", "wt");
	rtp_out_handle* p = malloc(sizeof(rtp_out_handle));
	p->log = fopen("encoder.rp_output_log.txt", "wt");
        p->binaryOut = fopen("encoder_rtp_ouput.264", "w+b");
    //    p->binaryOut = NULL;
        fprintf(p->log, "log oeffnet, open_file called\n");
	/*p->socket = openSocket(server, cport);
	if(p->socket <= 0)
	{
		printf("Fehler beim oeffnen des Sockets\n");
		return -1;
	}//*/
        srand(time(NULL));
	p->timestamp = time(NULL);
        if(!loop)
        {
            p->sequenznummer = rand()%10000;
            p->ssrc = rand()%100*time(NULL)+19;
            loop = 1;
            ssrc = p->ssrc;
            sequenznummer = p->sequenznummer;
        }
        else
        {
            p->ssrc = ssrc;
            p->sequenznummer = sequenznummer;
        }
        printf("ssrc: 0x%x, sequenznummer: %d\n", ssrc, sequenznummer);
        p->sps = p->pps = NULL;
	
	*p_handle = p;
	
	return 0;
}

static int set_param( hnd_t handle, x264_param_t *p_param )
{
	printf("set param:\n");
	print_x264_param(p_param);
    return 0;
}

static int write_headers( hnd_t handle, x264_nal_t *p_nal )
{
    rtp_out_handle* p = handle;

    // pps und sps im Handle speichern
    if(!p->sps && p_nal[0].i_type == NAL_SPS)
    {
        p->sps = (x264_nal_t*)malloc(sizeof(x264_nal_t));
        memcpy(p->sps, &p_nal[0], sizeof(x264_nal_t));
        // ignore leading start code 00 00 00 01
        p->sps->i_payload -= 4;
        p->sps->p_payload = malloc(p->sps->i_payload*sizeof(uint8_t));
        memcpy(p->sps->p_payload, p_nal[0].p_payload+4, p->sps->i_payload*sizeof(uint8_t));
    }

    if(!p->pps && p_nal[1].i_type == NAL_PPS)
    {
        p->pps = (x264_nal_t*)malloc(sizeof(x264_nal_t));
        memcpy(p->pps, &p_nal[1], sizeof(x264_nal_t));
        // ignore leading start code 00 00 00 01
        p->pps->i_payload -= 4;
        p->pps->p_payload = malloc(p->pps->i_payload*sizeof(uint8_t));
        memcpy(p->pps->p_payload, p_nal[1].p_payload+4, p->pps->i_payload*sizeof(uint8_t));
    }
	
    int size = p_nal[0].i_payload + p_nal[1].i_payload + p_nal[2].i_payload;
    return size;

    assert( p_nal[0].i_type == NAL_SPS );
    fprintf(p->log, "write SPS (%2d bytes) ", p_nal[0].i_payload);//0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n
    for(int i = 4; i < p_nal[0].i_payload; i++)
    {
        if(i >= p_nal[0].i_payload) break;
        fprintf(p->log, "0x%02x ", p_nal[0].p_payload[i]);
    }
    fprintf(p->log,"\n");

    // the first 4 bytes are the NAL size in bytes. skip this
   // if(sendFrame(p, p_nal[0].p_payload+4, p_nal[0].i_payload-4, NULL) < 0)  return -1;

    assert( p_nal[1].i_type == NAL_PPS );
    fprintf(p->log,"write PPS (%2d bytes) ", p_nal[1].i_payload);//0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n
    for(int i = 4; i < p_nal[1].i_payload; i++)
    {
        if(i >= p_nal[1].i_payload) break;
        fprintf(p->log,"0x%02x ", p_nal[1].p_payload[i]);
    }
    fprintf(p->log,"\n");
    
    /*
     fwrite( p_nal[0].p_payload, size, 1, p->binaryOut );
     if(g_FrameBuffer) clear_stack(g_FrameBuffer);
     *  //*/   
 //    g_FrameBuffer = stack_init(p_nal[0].p_payload+4, size-4);
     
    // the first 4 bytes are the NAL size in bytes. skip this
  // if(sendFrame(p, p_nal[0].p_payload+4, p_nal[0].i_payload-4, NULL) < 0)  return -1;
//    if(sendFrame(p, p_nal[1].p_payload+4, p_nal[1].i_payload-4, NULL) < 0)  return -1;
   //g_FrameBuffer = stack_init(p_nal[0].p_payload+4, p_nal[0].i_payload-4);
    //frame_to_stack(g_FrameBuffer, p_nal[1].p_payload, p_nal[1].i_payload);
     //if( fwrite( p_nal[0].p_payload, size, 1, (FILE*)handle ) )  

    assert( p_nal[2].i_type == NAL_SEI );
	
    /*if( fwrite( p_nal[0].p_payload, size, 1, stdout ) )
        return size;*/
    return size;
}

static int write_frame( hnd_t handle, uint8_t *p_nalu, int i_size, x264_picture_t *p_picture )
{
    //return i_size;
	rtp_out_handle* p = handle;
        //printf("write_frame: size: %d\n", i_size);
	//! TODO compiler warning: unknow conversion type
	fprintf(p->log, "write_frame: size: %6d, type: %c, pts: %6ld, dts: %6ld, nalu: 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", i_size, g_slice_types[p_picture->i_type], (long int)p_picture->i_pts, (long int)p_picture->i_dts, p_nalu[0], p_nalu[1], p_nalu[2], p_nalu[3], p_nalu[4], p_nalu[5]);
        if(!p_nalu)
        {
            fprintf(p->log, "encoder.rtp_output::write_frame: p_nalu is zero\n");
            return 0;
        }
        if(!p_picture)
        {
            fprintf(p->log, "encoder.rtp_output::write_frame: p_picture is zero\n");
            return 0;
        }
        if(i_size < 4)
        {
            fprintf(p->log, "encoder.rtp_output::write_frame: i_size is < 4\n");
            return 0;
        }
        
        fwrite( p_nalu, i_size, 1, p->binaryOut );
        unsigned* tag = (unsigned*)p_nalu;
        size_t s = i_size/sizeof(unsigned);
        fprintf(p->log, "size: %d: short: %d%d%d%d\n", i_size, tag[s-4], tag[s-3], tag[s-2], tag[s-1]);
        fflush(p->log);
        if(!g_FrameBuffer) 
            g_FrameBuffer = stack_init(p_nalu, i_size);
        else
           frame_to_stack(g_FrameBuffer, p_nalu, i_size);
        
        //*/
	//if(sendFrame(p, p_nalu+4, i_size-4, p_picture) < 0)  return -1;
        
       
//    if( fwrite( p_nalu, i_size, 1, (FILE*)handle ) )
  //      return i_size;
    //return -1;
       
    return i_size;
}

static int close_file( hnd_t handle, int64_t largest_pts, int64_t second_largest_pts )
{
    rtp_out_handle* p = handle;
	
    if( !handle || handle == stdout )
        return 0;
	
    //return 0;
    if(g_FrameBuffer)
    {
        if(mutex_lock(g_FrameBuffer->mutex))
            printf("encoder.rtp_output::close_file Fehler bei mutex_lock\n");
        
        SFrame_stack* t = g_FrameBuffer;
        g_FrameBuffer = NULL;
        
        if(mutex_unlock(t->mutex))
            printf("encoder.rtp_output::close_file Fehler bei mutex_unlock\n");
        
        clear_stack(t);
        t = NULL;
        printf("encoder.rtp_output::close_file stack cleared!\n");
    }
    
            
    fprintf(p->log, "Ende, close from handle and log-file\n");
    int ret = fclose(p->log);
    fclose(p->binaryOut);
	
   // closeSocket(p->socket);
    p->socket = 0;

    // Speicher für pps und sps im Handle wieder freigeben
    if(p->pps)
    {
        free(p->pps->p_payload);
        free(p->pps);
        p->pps = NULL;
    }
    if(p->sps)
    {
        free(p->sps->p_payload);
        free(p->sps);
        p->sps = NULL;
    }
    sequenznummer = p->sequenznummer;
	
    free(p);
    p = NULL;


    return ret;
}

const cli_output_t rtp_output = { open_file, set_param, write_headers, write_frame, close_file };

