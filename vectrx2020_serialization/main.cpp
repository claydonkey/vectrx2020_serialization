// ConsoleApplication1.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <vector>
#include <algorithm>
#include <iostream>
#include <string> 

#include <stdio.h>
#include <string.h>



#ifdef __GNUC__
#define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))
#endif

#ifdef _MSC_VER
#define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop))
#endif
#define SIZEOF_LEN 4
#define SIZEOF_HEADER 6
#define FLAG_COMPLETE 4
#define NUM_POINTS 20


using namespace std;


PACK(struct vec_t {
	uint16_t vec : 12;
});

PACK(struct point_t {
	uint16_t b : 8;
	uint16_t g : 8;
	uint16_t r : 8;
	uint16_t y : 12;
	uint16_t x : 12;
});

PACK(struct val_t {
	uint64_t merged : 64;
});

PACK(struct v_colors_t {
	uint32_t rgb : 32;
	uint8_t _pack8 : 8;
	uint8_t _pack4 : 4;
	uint8_t color : 1;
});

union dvg_vec {
	point_t pnt;
	uint64_t val;
	v_colors_t colors;
};

uint64_t ByteToint64(uint8_t a[], uint64_t* n) {
	memcpy((uint64_t*)n, a, 8);
	return *n;

}
uint32_t ByteToint32(uint8_t a[], uint32_t* n) {
	memcpy((uint32_t*)n, a, 4);
	return *n;

}
void int64ToByte(uint8_t a[], uint64_t n) {
	memcpy(a, &n, 8);

}
void int32ToByte(uint8_t a[], uint32_t n) {
	memcpy(a, &n, 4);

}
void Vecint64ToVecByte(uint8_t a[], uint64_t n) {
	memcpy(a, &n, 8);
	n = n >> 32;
	memcpy(a + 3, &n, 4);
}


uint64_t rand64(void)
{
	return rand() ^ ((uint64_t)rand() << 15) ^ ((uint64_t)rand() << 30) ^ ((uint64_t)rand() << 45) ^ ((uint64_t)rand() << 60);
}


uint8_t* out_meta_buff;
uint8_t* out_packed_points_buff;
void vectrx2020_serialize(vector<dvg_vec> out_points_buff, uint8_t** out_meta_buff, uint8_t** out_packed_points_buff);
void vectrx2020_deserialize(uint8_t* in_meta_buff, uint8_t* in_packed_points_buff);

int main()
{
	dvg_vec dvec = { 0 };

	dvec.pnt.x = 0xBAC;
	dvec.pnt.y = 0xFAF;
	dvec.pnt.r = 0xAA;
	dvec.pnt.g = 0xBB;
	dvec.pnt.b = 0xCC;
	dvec.colors.color = false;

	//serialize data

	vector<dvg_vec> out_points_buff;

	for (int i = 0; i < NUM_POINTS; i++)
	{
		dvg_vec point = { 0 };
		point.val = rand64();

		point.colors.color = (!(i % 2)) ? false : true;
		if (!point.colors.color)
			point.colors.rgb = 0;

		out_points_buff.push_back(point);
		printf("with_color=%x x=%x y=%x r=%x g=%x b=%x \n\r", point.colors.color, point.pnt.x, point.pnt.y, point.pnt.r, point.pnt.g, point.pnt.b);
	}


	vectrx2020_serialize(out_points_buff ,&out_meta_buff, &out_packed_points_buff);
	vectrx2020_deserialize(out_meta_buff, out_packed_points_buff);

	free(out_meta_buff);
		free(out_packed_points_buff);
}


void vectrx2020_serialize(vector<dvg_vec> out_points_buff , uint8_t** out_meta_buff , uint8_t** out_packed_points_buff) {

	printf("sending size: %lli\n\r", out_points_buff.size());

	vector<uint8_t> out_meta_points;
	vector<uint8_t> out_packed_pnts;

	int cmd = FLAG_COMPLETE;
	string header = "$cmd" + to_string(cmd) + " ";
	std::move(std::begin(header), std::end(header), std::back_inserter(out_meta_points));

	uint32_t out_bit_iter = 0;
	uint8_t meta_byte = 0;

	for (dvg_vec& pnt : out_points_buff)
	{

		uint8_t m_ar[8];

		int64ToByte(m_ar, pnt.val);

		if (pnt.colors.color) {
			out_packed_pnts.push_back(m_ar[2]); //r
			out_packed_pnts.push_back(m_ar[1]);  // g
			out_packed_pnts.push_back(m_ar[0]); // b

		}
		if (!(out_bit_iter % 8) && out_bit_iter) {
			out_meta_points.push_back(meta_byte);
			meta_byte = 0;
		}

		meta_byte |= pnt.colors.color << (out_bit_iter % 8);
		out_packed_pnts.push_back(m_ar[4]); //xy (backwards)
		out_packed_pnts.push_back(((m_ar[5] & 0x0F) << 4) | ((m_ar[6] & 0xF0) >> 4));
		out_packed_pnts.push_back(((m_ar[6] & 0x0F) << 4) | (m_ar[7] & 0x0F));
		out_bit_iter++;
	}
	out_meta_points.push_back(meta_byte);

	uint8_t meta_out_points_size[4];

	uint32_t out_packed_size = out_packed_pnts.size();

	int32ToByte(meta_out_points_size, out_points_buff.size());

	for (int i = 3; i >= 0; i--)
		out_meta_points.insert(out_meta_points.begin() + SIZEOF_HEADER, meta_out_points_size[i]);
	 

	*out_meta_buff  = (uint8_t*)malloc(out_meta_points.size() * sizeof(uint8_t));
	*out_packed_points_buff = (uint8_t*)malloc(out_packed_pnts.size() * sizeof(uint8_t));
 
	memcpy(*out_meta_buff, (uint8_t*)out_meta_points.data(), out_meta_points.size());
	memcpy(*out_packed_points_buff, (uint8_t*)out_packed_pnts.data(),  out_packed_pnts.size());

}

void vectrx2020_deserialize(uint8_t* in_meta_buff ,uint8_t* in_packed_points_buff ){

	uint32_t int_bit_iter = 0;
	uint8_t meta_byte=0;
	uint32_t with_color_size = 0;
	vector<dvg_vec> in_points;
	bool with_color[NUM_POINTS] = { 0 };
	int byte_iter;
	uint32_t in_p_size;
	ByteToint32(in_meta_buff + +SIZEOF_HEADER, &in_p_size);

	for (byte_iter = 0; byte_iter < in_p_size - 8; byte_iter += 8)
	{
		 meta_byte = *(in_meta_buff + (SIZEOF_HEADER + SIZEOF_LEN) + byte_iter / 8);
		for (int_bit_iter = 0; int_bit_iter < 8; int_bit_iter++) {
			uint8_t bit = 1 & meta_byte >> int_bit_iter;
			with_color[int_bit_iter + byte_iter] = (bit);
			with_color_size++;
		}
	}

	if (in_p_size % 8)
		 meta_byte = *(in_meta_buff + (SIZEOF_HEADER + SIZEOF_LEN + 1) + byte_iter / 8);

	for (int_bit_iter = 0; int_bit_iter < in_p_size % 8; int_bit_iter++) {
		uint8_t bit = 1 & meta_byte >> int_bit_iter;
		with_color[int_bit_iter + byte_iter] = (bit);
		with_color_size++;
	}
	if (with_color_size != in_p_size)
		printf("error\n\r");



	uint8_t* start_points_buff = in_packed_points_buff;
	if (in_packed_points_buff != NULL) {
		for (int pts = 0; pts < with_color_size; pts++)
		{
			dvg_vec point;
			point.colors.color = with_color[pts];
			point.pnt.r = 0;
			point.pnt.g = 0;
			point.pnt.b = 0;

			if (point.colors.color)
			{
				point.pnt.r = *(in_packed_points_buff);
				point.pnt.g = *(in_packed_points_buff + 1);
				point.pnt.b = *(in_packed_points_buff + 2);

				in_packed_points_buff += 3;
			}

			point.pnt.y = ((*(in_packed_points_buff + 1) & 0xF0) << 4) + *(in_packed_points_buff);
			point.pnt.x = ((*(in_packed_points_buff + 2) & 0x0F) << 8) + ((*(in_packed_points_buff + 2) & 0xF0) >> 4) + ((*(in_packed_points_buff + 1) & 0x0F) << 4);

			in_packed_points_buff += 3;

			//DO SOMETHING
			printf("with_color=%x x=%x y=%x r=%x g=%x b=%x \n\r", point.colors.color, (point.pnt.x), point.pnt.y, point.pnt.r, point.pnt.g, point.pnt.b);
			//OR STORE
			//in_points.push_back(point);
		}


	//	for (dvg_vec& pnt : in_points)
	//	{
	//		printf("with_color=%x x=%x y=%x r=%x g=%x b=%x \n\r", pnt.colors.color, (pnt.pnt.x), pnt.pnt.y, pnt.pnt.r, pnt.pnt.g, pnt.pnt.b);
	//	}
	//	printf("results size: %lli\n\r", in_points.size());

	}

	in_packed_points_buff = start_points_buff;

}