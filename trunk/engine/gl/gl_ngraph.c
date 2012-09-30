/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// gl_ngraph.c

#include "quakedef.h"
#include "shader.h"

extern qbyte		*draw_chars;				// 8*8 graphic characters

static texid_t	netgraphtexture;	// netgraph texture
static shader_t *netgraphshader;

#define NET_GRAPHHEIGHT 32

static	qbyte ngraph_texels[NET_GRAPHHEIGHT][NET_TIMINGS];

static void R_LineGraph (int x, int h)
{
	int		i;
	int		s;
	int		color;

	s = NET_GRAPHHEIGHT;

	if (h == 10000)
		color = 0x6f;	// yellow
	else if (h == 9999)
		color = 0x4f;	// red
	else if (h == 9998)
		color = 0xd0;	// blue
	else
		color = 0xfe;	// white

	if (h>s)
		h = s;
	
	for (i=0 ; i<h ; i++)
		if (i & 1)
			ngraph_texels[NET_GRAPHHEIGHT - i - 1][x] = 0xff;
		else
			ngraph_texels[NET_GRAPHHEIGHT - i - 1][x] = (qbyte)color;

	for ( ; i<s ; i++)
		ngraph_texels[NET_GRAPHHEIGHT - i - 1][x] = (qbyte)0xff;
}

/*
static void Draw_CharToNetGraph (int x, int y, int num)
{
	int		row, col;
	qbyte	*source;
	int		drawline;
	int		nx;

	if (!draw_chars)
		return;

	row = num>>4;
	col = num&15;
	source = draw_chars + (row<<10) + (col<<3);

	for (drawline = 8; drawline; drawline--, y++)
	{
		for (nx=0 ; nx<8 ; nx++)
			if (source[nx] != 255)
				ngraph_texels[y][nx+x] = 0x60 + source[nx];
		source += 128;
	}
}
*/

/*
==============
R_NetGraph
==============
*/
void R_NetGraph (void)
{
	int		a, x, i, y;
	int lost;
	char st[80];
	unsigned	ngraph_pixels[NET_GRAPHHEIGHT][NET_TIMINGS];

	x = 0;
	lost = CL_CalcNet();
	for (a=0 ; a<NET_TIMINGS ; a++)
	{
		i = (cls.netchan.outgoing_sequence-a) & NET_TIMINGSMASK;
		R_LineGraph (NET_TIMINGS-1-a, packet_latency[i]);
	}

	// now load the netgraph texture into gl and draw it
	for (y = 0; y < NET_GRAPHHEIGHT; y++)
		for (x = 0; x < NET_TIMINGS; x++)
			ngraph_pixels[y][x] = d_8to24rgbtable[ngraph_texels[y][x]];

	x =	((vid.width - 320)>>1);
	x=-x;
	y = vid.height - sb_lines - 24 - NET_GRAPHHEIGHT - 1;

	M_DrawTextBox (x, y, NET_TIMINGS/8, NET_GRAPHHEIGHT/8 + 1);
	y += 8;

	sprintf(st, "%3i%% packet loss", lost);
	Draw_FunString(8, y, st);
	y += 8;

	R_Upload(netgraphtexture, "***netgraph***", TF_RGBA32, ngraph_pixels, NULL, NET_TIMINGS, NET_GRAPHHEIGHT, IF_NOMIPMAP|IF_NOPICMIP);
	x=8;
	R2D_Image(x, y, NET_TIMINGS, NET_GRAPHHEIGHT, 0, 0, 1, 1, netgraphshader);
}

void R_FrameTimeGraph (int frametime)
{
	int		a, x, i, y;
	int lost;
	char st[80];
	unsigned	ngraph_pixels[NET_GRAPHHEIGHT][NET_TIMINGS];

	static int timehistory[NET_TIMINGS];
	static int findex;

	timehistory[findex++&NET_TIMINGSMASK] = frametime;

	x = 0;
	lost = CL_CalcNet();
	for (a=0 ; a<NET_TIMINGS ; a++)
	{
		i = (findex-a) & NET_TIMINGSMASK;
		R_LineGraph (NET_TIMINGS-1-a, timehistory[i]);
	}

	// now load the netgraph texture into gl and draw it
	for (y = 0; y < NET_GRAPHHEIGHT; y++)
		for (x = 0; x < NET_TIMINGS; x++)
			ngraph_pixels[y][x] = d_8to24rgbtable[ngraph_texels[y][x]];

	x =	((vid.width - 320)>>1);
	x=-x;
	y = vid.height - sb_lines - 24 - NET_GRAPHHEIGHT - 1;

	M_DrawTextBox (x, y, NET_TIMINGS/8, NET_GRAPHHEIGHT/8 + 1);
	y += 8;

	sprintf(st, "%3i%% packet loss", lost);
	Draw_FunString(8, y, st);
	y += 8;

	R_Upload(netgraphtexture, "***netgraph***", TF_RGBA32, ngraph_pixels, NULL, NET_TIMINGS, NET_GRAPHHEIGHT, IF_NOMIPMAP|IF_NOPICMIP);
	x=8;
	R2D_Image(x, y, NET_TIMINGS, NET_GRAPHHEIGHT, 0, 0, 1, 1, netgraphshader);
}

void R_NetgraphInit(void)
{
	TEXASSIGN(netgraphtexture, R_AllocNewTexture("***netgraph***", NET_TIMINGS, NET_GRAPHHEIGHT, IF_NOMIPMAP));
	netgraphshader = R_RegisterShader("netgraph",
		"{\n"
			"program default2d\n"
			"{\n"
				"map $diffuse\n"
				"blendfunc blend\n"
			"}\n"
		"}\n"
		);
	netgraphshader->defaulttextures.base = netgraphtexture;
}
