#include "H2642RTMP.h"


CH2642RTMP::CH2642RTMP()
{
	//std::cout << "CH2642RTMP::CH2642RTMP()" << std::endl;

	m_H264 = new char[m_H264Length];
	memset(m_H264, 0, m_H264Length);
	m_SendH264 = new char[m_SendH264Length];
	memset(m_SendH264, 0, m_SendH264Length);
}


CH2642RTMP::~CH2642RTMP()
{
	//std::cout << "CH2642RTMP::~CH2642RTMP()" << std::endl;

	if (outputContext)
	{
		avformat_close_input(&outputContext);
		outputContext = nullptr;
	}
	if (packet)
	{
		av_packet_free(&packet);
		packet = nullptr;
	}
	if (nullptr != m_H264)
	{
		delete[] m_H264;
		m_H264 = nullptr;
	}
	if (nullptr != m_SendH264)
	{
		delete[] m_SendH264;
		m_SendH264 = nullptr;
	}
}


int U(int iBitCount, char* bData, int &iStartBit)
{
	int iRet = 0;
	for (int i = 0; i < iBitCount; i++)
	{
		iRet = iRet << 1;
		if ((0x80 >> (iStartBit % 8)) == (bData[iStartBit / 8] & (0x80 >> (iStartBit % 8))))
		{
			iRet += 1;
		}
		iStartBit++;
	}
	return iRet;
}

uint32_t Ue(char* bData, int Length, int &iStartBit)
{
	//计算0bit的个数
	int nZeroNum = 0;
	while (iStartBit < Length * 8)
	{
		if ((0x80 >> (iStartBit % 8)) == (bData[iStartBit / 8] & (0x80 >> (iStartBit % 8)))) //&:按位与，%取余
		{
			break;
		}
		nZeroNum++;
		iStartBit++;
	}
	nZeroNum = nZeroNum + 1;
	//计算结果
	uint32_t dwRet = 0;
	for (uint32_t i = 0; i < nZeroNum; i++)
	{
		dwRet <<= 1;
		if ((0x80 >> (iStartBit % 8)) == (bData[iStartBit / 8] & (0x80 >> (iStartBit % 8))))
		{
			dwRet += 1;
		}
		iStartBit++;
	}
	return dwRet - 1;
}

uint32_t Se(char* bData, int Length, int &iStartBit)
{
	//计算0bit的个数
	int nZeroNum = 0;
	while (iStartBit < Length * 8)
	{
		if ((0x80 >> (iStartBit % 8)) == (bData[iStartBit / 8] & (0x80 >> (iStartBit % 8)))) //&:按位与，%取余
		{
			break;
		}
		nZeroNum++;
		iStartBit++;
	}
	//计算结果
	uint32_t dwRet = 0;
	for (uint32_t i = 0; i < nZeroNum; i++)
	{
		dwRet <<= 1;
		if ((0x80 >> (iStartBit % 8)) == (bData[iStartBit / 8] & (0x80 >> (iStartBit % 8))))
		{
			dwRet += 1;
		}
		iStartBit++;
	}
	if ((0x80 >> (iStartBit % 8)) == (bData[iStartBit / 8] & (0x80 >> (iStartBit % 8))))
	{
		dwRet = 0 - dwRet;
	}
	iStartBit++;
	return dwRet;
}


bool CH2642RTMP::Init(const char* rtmpUrl)
{
	m_rtmpUrl = rtmpUrl;
	packet = av_packet_alloc();
	if (nullptr == packet)
		return false;

	int ret = avformat_alloc_output_context2(&outputContext, nullptr, "flv", m_rtmpUrl);
	if (ret < 0)
		return false;

	return true;
}

void CH2642RTMP::AddData(char * pData, int iSize)
{
	if (iSize <= 4)
	{
		return;
	}
	bool bCase1 = (pData[0] & 0x1F) == 0 && (pData[1] & 0x1F) == 0 && (pData[2] & 0x1F) == 1;
	bool bCase2 = (pData[0] & 0x1F) == 0 && (pData[1] & 0x1F) == 0 && (pData[2] & 0x1F) == 0 && (pData[3] & 0x1F) == 1;
	if (false == bCase1 && false == bCase2)
	{
		return;
	}
	if ((m_CurrentH264Length + iSize) > m_H264Length)
	{
		m_CurrentH264Length = 0;
	}
	if (iSize >= m_H264Length)
	{
		fprintf(stderr, "AddData iSize too big size=%d\n", iSize);
		return;
	}
	memcpy(m_H264 + m_CurrentH264Length, pData, iSize);
	m_CurrentH264Length = m_CurrentH264Length + iSize;

	int iH264Length = 0;
	while (GetOneH264Length(iH264Length))
	{
		if (((m_H264[3] & 0x1F) == 6) ||
			((m_H264[4] & 0x1F) == 6) && (m_H264[3] & 0x1F) == 1)
		{
			memcpy(m_SEI, m_H264, iH264Length);
			m_SEILength = iH264Length;
		}
		else if (((m_H264[3] & 0x1F) == 7) ||
			((m_H264[4] & 0x1F) == 7) && (m_H264[3] & 0x1F) == 1)
		{
			memcpy(m_Sps, m_H264, iH264Length);
			m_SpsLength = iH264Length;
		}
		else if (((m_H264[3] & 0x1F) == 8) ||
			((m_H264[4] & 0x1F) == 8) && (m_H264[3] & 0x1F) == 1)
		{
			memcpy(m_Pps, m_H264, iH264Length);
			m_PpsLength = iH264Length;
		}
		else
		{
			SendRtmp(m_H264, iH264Length);
		}
		if (m_CurrentH264Length >= iH264Length)
		{
			m_CurrentH264Length = m_CurrentH264Length - iH264Length;
			memcpy(m_H264, m_H264 + iH264Length, m_CurrentH264Length);
		}
	}
}

void CH2642RTMP::SetInterval(int interval)
{
	m_interlval = interval;
}

void CH2642RTMP::DataPreproce(char * pData, int iSize)
{
	if (iSize > 1024 * 1024)
		return;

	if (m_CacheSize + iSize > 1024 * 1024)
	{
		memset(m_Cache, 0, 1024*1024);
		m_CacheSize = 0;
	}
	memcpy(m_Cache + m_CacheSize, pData, iSize);
	m_CacheSize += iSize;

	if (m_CacheSize <= 8)
		return;
	int i = 4;
	while (i < m_CacheSize - 4)
	{
		if ((m_Cache[i] == 0x00 && m_Cache[i + 1] == 0x00 && m_Cache[i + 2] == 0x00 && m_Cache[i + 3] == 0x01) ||
			(m_Cache[i] == 0x00 && m_Cache[i + 1] == 0x00 && m_Cache[i + 2] == 0x01))
		{
			AddData(m_Cache, i);
			m_CacheSize -= i;
			memcpy(m_Cache, m_Cache + i, m_CacheSize);
			i = 4;
			continue;
		}
		i++;
	}
}

bool CH2642RTMP::InitCodec()
{
	int width = 0;
	int height = 0;
	int iNext = 4;
	if ((m_Sps[2] & 0x1F) == 1)
	{
		iNext = 3;
	}
	if (false == H264_decode_sps(m_Sps + iNext, m_SpsLength - iNext, width, height))
	{
		return false;
	}
	auto pInCodec = avcodec_find_decoder(AV_CODEC_ID_H264);
	AVStream *stream = avformat_new_stream(outputContext, pInCodec);
	if (NULL == stream) {
		fprintf(stderr, "Failed avformat_new_stream stream\n");
		return false;
	}
	stream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
	stream->codecpar->codec_id = AV_CODEC_ID_H264;
	stream->codecpar->codec_tag = 0;
	stream->codecpar->width = width;
	stream->codecpar->height = height;
	stream->codecpar->profile = 100;
	stream->codecpar->extradata = (uint8_t *)av_malloc(m_SpsLength + m_PpsLength);
	memcpy(stream->codecpar->extradata, m_Sps, m_SpsLength);
	memcpy(stream->codecpar->extradata + m_SpsLength, m_Pps, m_PpsLength);
	stream->codecpar->extradata_size = m_SpsLength + m_PpsLength;
	av_dump_format(outputContext, 0, m_rtmpUrl, 1);

	int ret = avio_open(&outputContext->pb, m_rtmpUrl, AVIO_FLAG_WRITE);
	if (ret < 0)
	{
		fprintf(stderr, "avio_open2 error\n");
		return false;
	}

	ret = avformat_write_header(outputContext, nullptr);
	if (ret < 0)
	{
		fprintf(stderr, "avformat_write_header error\n");
		return false;
	}
	return true;
}

bool CH2642RTMP::H264_decode_sps(char * bData, int Length, int & width, int & height)
{
	int StartBit = 0;
	int forbidden_zero_bit = U(1, bData, StartBit);
	int nal_ref_idc = U(2, bData, StartBit);
	int nal_unit_type = U(5, bData, StartBit);
	if (nal_unit_type == 7)
	{
		int profile_idc = U(8, bData, StartBit);
		int constraint_set0_flag = U(1, bData, StartBit);//(buf[1] & 0x80)>>7;
		int constraint_set1_flag = U(1, bData, StartBit);//(buf[1] & 0x40)>>6;
		int constraint_set2_flag = U(1, bData, StartBit);//(buf[1] & 0x20)>>5;
		int constraint_set3_flag = U(1, bData, StartBit);//(buf[1] & 0x10)>>4;
		int reserved_zero_4bits = U(4, bData, StartBit);
		int level_idc = U(8, bData, StartBit);
		auto seq_parameter_set_id = Ue(bData, Length, StartBit);
		auto chroma_format_idc = 0;
		if (profile_idc == 100 || profile_idc == 110 ||
			profile_idc == 122 || profile_idc == 144)
		{
			chroma_format_idc = Ue(bData, Length, StartBit);
			if (chroma_format_idc == 3)
			{
				int residual_colour_transform_flag = U(1, bData, StartBit);
			}
			auto bit_depth_luma_minus8 = Ue(bData, Length, StartBit);
			auto bit_depth_chroma_minus8 = Ue(bData, Length, StartBit);
			int qpprime_y_zero_transform_bypass_flag = U(1, bData, StartBit);
			int seq_scaling_matrix_present_flag = U(1, bData, StartBit);

			int seq_scaling_list_present_flag[8];
			if (1 == seq_scaling_matrix_present_flag)
			{
				for (int i = 0; i < 8; i++)
				{
					seq_scaling_list_present_flag[i] = U(1, bData, StartBit);
				}
			}
		}
		auto log2_max_frame_num_minus4 = Ue(bData, Length, StartBit);
		auto pic_order_cnt_type = Ue(bData, Length, StartBit);
		if (pic_order_cnt_type == 0)
		{
			auto log2_max_pic_order_cnt_lsb_minus4 = Ue(bData, Length, StartBit);
		}
		else if (pic_order_cnt_type == 1)
		{
			int delta_pic_order_always_zero_flag = U(1, bData, StartBit);
			int offset_for_non_ref_pic = Se(bData, Length, StartBit);
			int offset_for_top_to_bottom_field = Se(bData, Length, StartBit);
			auto num_ref_frames_in_pic_order_cnt_cycle = Ue(bData, Length, StartBit);

			int offset_for_ref_frame[1024];
			for (int i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++)
				offset_for_ref_frame[i] = Se(bData, Length, StartBit);
		}
		auto num_ref_frames = Ue(bData, Length, StartBit);
		int gaps_in_frame_num_value_allowed_flag = U(1, bData, StartBit);
		auto pic_width_in_mbs_minus1 = Ue(bData, Length, StartBit);
		auto pic_height_in_map_units_minus1 = Ue(bData, Length, StartBit);


		int frame_mbs_only_flag = U(1, bData, StartBit);
		if (0 == frame_mbs_only_flag)
		{
			int mb_adaptive_frame_field_flag = U(1, bData, StartBit);
		}
		int direct_8x8_inference_flag = U(1, bData, StartBit);
		int frame_cropping_flag = U(1, bData, StartBit);

		auto frame_crop_left_offset = 0;
		auto frame_crop_right_offset = 0;
		auto frame_crop_top_offset = 0;
		auto frame_crop_bottom_offset = 0;

		if (1 == frame_cropping_flag)
		{
			frame_crop_left_offset = Ue(bData, Length, StartBit);
			frame_crop_right_offset = Ue(bData, Length, StartBit);
			frame_crop_top_offset = Ue(bData, Length, StartBit);
			frame_crop_bottom_offset = Ue(bData, Length, StartBit);
		}
		int vui_parameter_present_flag = U(1, bData, StartBit);
		if (1 == vui_parameter_present_flag)
		{
			int aspect_ratio_info_present_flag = U(1, bData, StartBit);
			if (1 == aspect_ratio_info_present_flag)
			{
				int aspect_ratio_idc = U(8, bData, StartBit);
				if (aspect_ratio_idc == 255)
				{
					int sar_width = U(16, bData, StartBit);
					int sar_height = U(16, bData, StartBit);
				}
			}
			int overscan_info_present_flag = U(1, bData, StartBit);
			if (1 == overscan_info_present_flag)
			{
				int overscan_appropriate_flagu = U(1, bData, StartBit);
			}
			int video_signal_type_present_flag = U(1, bData, StartBit);
			if (1 == video_signal_type_present_flag)
			{
				int video_format = U(3, bData, StartBit);
				int video_full_range_flag = U(1, bData, StartBit);
				int colour_description_present_flag = U(1, bData, StartBit);
				if (1 == colour_description_present_flag)
				{
					int colour_primaries = U(8, bData, StartBit);
					int transfer_characteristics = U(8, bData, StartBit);
					int matrix_coefficients = U(8, bData, StartBit);
				}
			}
			int chroma_loc_info_present_flag = U(1, bData, StartBit);
			if (1 == chroma_loc_info_present_flag)
			{
				auto chroma_sample_loc_type_top_field = Ue(bData, Length, StartBit);
				auto chroma_sample_loc_type_bottom_field = Ue(bData, Length, StartBit);
			}
			int timing_info_present_flag = U(1, bData, StartBit);

			if (1 == timing_info_present_flag)
			{
				int num_units_in_tick = U(32, bData, StartBit);
				int time_scale = U(32, bData, StartBit);
				int fixed_frame_rate_flag = U(1, bData, StartBit);
			}

		}

		// 宽高计算公式
		width = ((int)pic_width_in_mbs_minus1 + 1) * 16;
		height = (2 - (int)frame_mbs_only_flag) * ((int)pic_height_in_map_units_minus1 + 1) * 16;

		if (1 == frame_cropping_flag)
		{
			int crop_unit_x;
			int crop_unit_y;
			if (0 == chroma_format_idc) // monochrome
			{
				crop_unit_x = 1;
				crop_unit_y = 2 - frame_mbs_only_flag;
			}
			else if (1 == chroma_format_idc) // 4:2:0
			{
				crop_unit_x = 2;
				crop_unit_y = 2 * (2 - frame_mbs_only_flag);
			}
			else if (2 == chroma_format_idc) // 4:2:2
			{
				crop_unit_x = 2;
				crop_unit_y = 2 - frame_mbs_only_flag;
			}
			else // 3 == sps.chroma_format_idc   // 4:4:4
			{
				crop_unit_x = 1;
				crop_unit_y = 2 - frame_mbs_only_flag;
			}

			width -= crop_unit_x * ((int)frame_crop_left_offset + (int)frame_crop_right_offset);
			height -= crop_unit_y * ((int)frame_crop_top_offset + (int)frame_crop_bottom_offset);
		}
		return true;
	}
	else
	{
		return false;
	}
}

bool CH2642RTMP::GetOneH264Length(int & iLength)
{
	bool OneFrame = false;
	if (m_CurrentH264Length <= 8)
	{
		return false;
	}
	for (int i = 4; i < m_CurrentH264Length - 4; i++)
	{
		if ((m_H264[i] == 0x00 && m_H264[i + 1] == 0x00 && m_H264[i + 2] == 0x00 && m_H264[i + 3] == 0x01) ||
			(m_H264[i] == 0x00 && m_H264[i + 1] == 0x00 && m_H264[i + 2] == 0x01))
		{
			OneFrame = true;
			iLength = i;
			break;
		}
	}
	if (OneFrame)
	{
		return true;
	}
	return false;
}

void CH2642RTMP::SendRtmp(char * pData, int iSize)
{
	if (0 == m_SpsLength || 0 == m_PpsLength)
	{
		return;
	}
	if (false == m_InitCodec)
	{
		m_InitCodec = true;
		m_InitCodecOK = InitCodec();
	}
	if (false == m_InitCodecOK)
	{
		return;
	}
	int iLength = 0;
	if (((pData[3] & 0x1F) == 5) ||
		((pData[4] & 0x1F) == 5) && (pData[3] & 0x1F) == 1)
	{
		memcpy(m_SendH264, m_Sps, m_SpsLength);
		memcpy(m_SendH264 + m_SpsLength, m_Pps, m_PpsLength);
		memcpy(m_SendH264 + m_SpsLength + m_PpsLength, m_SEI, m_SEILength);
		memcpy(m_SendH264 + m_SpsLength + m_PpsLength + m_SEILength, pData, iSize);
		iLength = m_SpsLength + m_PpsLength + m_SEILength + iSize;
	}
	else
	{
		memcpy(m_SendH264, pData, iSize);
		iLength = iSize;
	}
	int ret = 0;
	packet->size = iLength;
	packet->data = (uint8_t *)av_malloc(packet->size);
	memcpy(packet->data, m_SendH264, iLength);
	ret = av_packet_from_data(packet, packet->data, packet->size);
	if (ret < 0)
	{
		fprintf(stderr, "av_packet_from_data error \n");
		av_free(packet->data);
		return;
	}
	packet->pts = m_lastpts;
	packet->dts = m_lastpts;
	m_lastpts = m_lastpts + m_interlval;
	packet->duration = m_interlval;
	packet->flags = 1;
	ret = av_interleaved_write_frame(outputContext, packet);
	if (ret < 0)
	{
		fprintf(stderr, "avcodec_send_packet error \n");

		memset(m_H264, 0, m_H264Length);
		m_CurrentH264Length = 0;
		memset(m_SendH264, 0, m_SendH264Length);
	}
}
