# debra software

debra sofware stanowi oprogramowanie dla procesorów nrf52 zgodnych z technologią BLE 5.1. 
 

## Description

Oprogramowanie składa się z wielu modułów, ich opis zamieszczono pomiżej.

Program wykonuje sie w funkcji main(), gdzie w pierwszej kolejności następuje inicjalizacja modułów:

```C
	IF_Initialization();				//Inicjalizacja UART
	PROTOCOL_Initialization();		//Inicjalizacja protokołu do komunikacji z aplikacjami do wizualizacji
	BLE_Initialization();				//Inicjalizcaja BLE
	AOA_Initialization();				//Inicjalizacja algorytmu AoA
```

Następnie w pętli while() w sposób ciagły realizowana jest ich obsługa.

```C
	while(1)
	{
		AOA_Handling();				//Obsługa AoA
		PROTOCOL_Handling();			//Obsługa komunikacji z aplikacjami do wizualizacji
	}
```

W funkcji AOA_Handling() realizowana jest najwazniejsza funkcjonalność oprogramowania. Poniżej załączono ciało funkcji oraz dodano istotne komentarze.

```C
void AOA_Handling(void)
{
#if LOG_DF_PACKET == 0														//Flaga określająca, czy logujemy surowe próbki IQ. Jeśli tak to wyłączamy obsługe AoA z poziomu NRF
	aoa_configuration *config = &AOA_CONFIG;
	aoa_data *aoa = &AOA;

	struct df_packet df_packet = {0};										//Pakiet zawierający próbki IQ
	memset(&df_packet, 0, sizeof(df_packet));
	k_msgq_get(&df_packet_msgq, &df_packet, K_NO_WAIT);					//Pobranie próbek IQ z kolejki, która inicjalizowana jest w stosie
	if (df_packet.hdr.length == 0) { return; }
	if (!aoa->pdda->init_flag) { aoa->pdda->init(); }

	aoa->frequency = df_packet.hdr.frequency;
	int time = k_uptime_get();
	aoa->loop_time = time - aoa->last_time;
	aoa->last_time = time;

	uint16_t req_len = config->reference_period*1000/config->df_r + ((config->antennas_num-1)*(config->df_sw*1000 / config->df_s))/2;
	float module = 0.0f;
	uint16_t index = 0;

	aoa->raw_length = df_packet.hdr.length;
	for(uint16_t i=0; i<aoa->raw_length; i++)								//Przechwycenie próbek
	{
		aoa->raw_iq[i*2] = (float)(df_packet.data[i].iq.q);
		aoa->raw_iq[i*2+1] = (float)(df_packet.data[i].iq.i);

		if (df_get_sample_antenna_ids()[i] != 0xFF)
		{
			if (index < req_len)
			{
//				if ((df_packet.data[i].iq.i == (int16_t)0x8000) || (df_packet.data[i].iq.q == (int16_t)0x8000)) { return; }
				module = 0;
				arm_cmplx_mag_f32(&aoa->raw_iq[i*2], &module, 1);			//Normalizacja IQ 
				aoa->iq[index*2] = aoa->raw_iq[i*2] / module;
				aoa->iq[index*2+1] = aoa->raw_iq[i*2+1] / module;
				index++;
			}
		}
	}
	aoa->length = index;
	uint16_t buff_length = (aoa->length*2)*sizeof(*aoa->temp_iq);

	memcpy(aoa->temp_iq, aoa->iq, buff_length);
	FREQ_Handling(aoa->temp_iq, aoa->iq, aoa->length);					//Korekcja częstotliwości - poprawienie IQ w dziedzicie czasu - normalizacja do 250kHz
	PDDA_Handling(aoa->iq);													//Metoda PDDA - wyliczenia elewacji i azymutu
#if (__USE_KALMAN == 1)
	aoa->skf->dt = aoa->loop_time/1000.0f;
	aoa->skf->arg = aoa->pdda->quaternion;
	if (!aoa->skf->init_flag) { aoa->skf->init(); }
	SKF_Handling();															//Filtr Kalmana - Wygładzenie wyników
#endif
#if (__USE_SERIAL == 1)														//Flaga włączająca logowanie - używane przy uruchomieniu skryptu run_log.sh lub przy aplikacji QT
	aoa->protocol->tx_packet.frequency = aoa->frequency;
	aoa->protocol->tx_packet.iq_length = aoa->raw_length;
	aoa->protocol->tx_packet.pdda_elevation = aoa->pdda->elevation;
	aoa->protocol->tx_packet.pdda_azimuth = aoa->pdda->azimuth;
	aoa->protocol->tx_packet.kalman_elevation = aoa->skf->elevation;
	aoa->protocol->tx_packet.kalman_azimuth = aoa->skf->azimuth;
	aoa->protocol->tx_status = PROTOCOL_REQUEST_STRING_SEND;
#else
	printk("V: %d %d %d %d\r\n", (int)(aoa->skf->array.V[0]*10), (int)(aoa->skf->array.V[1]*10), (int)(aoa->skf->array.V[2]*10), (int)(aoa->skf->array.V[3]*10));
	printk("pdda: %d %d\r\n", (int)aoa->pdda->elevation, (int)aoa->pdda->azimuth);
	printk("kalman: %d %d\r\n", (int)aoa->skf->elevation, (int)aoa->skf->azimuth);
#endif
#endif
}
```

W zaprezentowanym powyżej ciele funkcji znajdziemy wywołania innych ważnych funkcji, które analogicznie omówiono poniżej.
W pierwszej kolejności omówiono funkcję FREQ_Handling(), która odpowiedzialna jest za korekcje częstotliwości.

```C
void FREQ_Handling(float *iq_in, float *iq_out, uint16_t length)
{
	aoa_configuration *config = &AOA_CONFIG;
	freq_vector *freq = &FREQ;

	memset(&FREQ, 0, sizeof(FREQ));

	uint8_t M = config->antennas_num;														//Pobranie parametrów konfiguracyjnych
	uint8_t N = config->snapshot_len;
	uint16_t RR = (uint16_t)(config->reference_period*1000 / config->df_r);
	uint16_t SR = (uint16_t)(config->df_sw*1000 / config->df_s);
	uint16_t period = (uint16_t)(1000000 / config->frequency);

	uint16_t i_index = 0;
	uint16_t index = 0;
	for(uint16_t i=0; i<RR; i++)																//Stworzenie listy indeksów próbek IQ biorących udział w korekcji
	{
		freq->index_tab[index++] = i;
	}
	for(uint16_t i=0; i<(M-1)*N; i++)
	{
		i_index = (uint16_t)(RR+SR/2 + i*SR);
		for(uint16_t j=0; j<(uint16_t)(SR/2); j++)
		{
			freq->index_tab[index++] = (uint16_t)(i_index + j);
		}
	}

	float temp = 0.0f;
	float complex z1 = 0, z2 = 0;
	uint16_t size = (uint16_t)(RR/2);

	for(uint16_t i=0; i<size; i++)																																			
	{
		z1 = iq_in[i*2] + iq_in[i*2+1]*I;													//Pobranie próbki IQ z pierwszej połówki sygnały referencyjnego
		z2 = iq_in[((RR/2)+i)*2] + iq_in[((RR/2)+i)*2+1]*I;								//Pobranie próbki IQ z drugiej połówki sygnały referencyjnego
		temp = cargf(z1) - cargf(z2);														//Obliczenie różnicy faz tych próbek

		if (temp > PI)
		{
			temp -= 2*PI;
		}
		else if (temp < -PI)
		{
			temp += 2*PI;
		}

		freq->phase_iq[i] = temp;
	}

	float phase_iq_sum = 0.0f;
	for(uint16_t i=0; i<size; i++)
	{
		phase_iq_sum += freq->phase_iq[i];
	}

	float phase_average = phase_iq_sum / size;												//Obliczenie średniej z różnic faz między próbkami IQ
	float phase_error = phase_average / period / (1000 / config->df_r);					//Obliczenie rzeczywistej różnicy fazy pomiędzy sąsiadującymi próbkami IQ

	for(uint16_t i=0; i<length; i++)														//Obliczenie różnicy faz dla każdej próbki IQ względem pierwszej
	{
		freq->phase[i] += freq->index_tab[i] * phase_error;								
	}

	float iq[2] = {0};
	for(uint16_t i=0; i<length; i++)														//Przesunięcie próbek o wyliczone różnice faz
	{
		iq[0] = arm_cos_f32(freq->phase[i]);
		iq[1] = arm_sin_f32(freq->phase[i]);
		arm_cmplx_mult_cmplx_f32(&iq_in[2*i], iq, &iq_out[2*i], 1);
	}
}
```

Kolejna funkcja to PDDA_Handling(), która wylicza kąty odbieranego sygnału na podstawie próbek IQ.

```C
void PDDA_Handling(float *iq_samples)
{
	aoa_configuration *config = &AOA_CONFIG;
	pdda_vector *pdda = &PDDA;
	pdda_array *arr = &pdda->array;
	pdda_matrix *mat = &pdda->matrix;

	memset(&pdda->max, 0, sizeof(pdda->max));

	float EXP = 2*PI * ((config->array_distance*AOA_WAVE_LAMBDA)/AOA_WAVE_LAMBDA);		//Pobranie parametrów konfiguracyjnych
	uint16_t RR = (uint16_t)(config->reference_period*1000 / config->df_r);
	uint16_t SR = (uint16_t)(config->df_sw*1000 / config->df_s);
	uint8_t M = config->antennas_num;
	uint8_t S = config->matrix_size;
	uint8_t N = config->snapshot_len;
	uint16_t SW = config->df_sw;

	uint16_t r_index = 0;
	uint16_t n_index = 0;
	uint16_t m_index = 0;
	uint16_t s_index = 0;
	uint16_t size = SR/2;

	for (uint16_t n=0; n<N; n++)																//Obliczenie macierzy X, rozmieszczenie próbek IQ w odpowiednie miejsca 
	{
		if (M%2 == 0)
		{
			if (n%2 == 0)
			{
				r_index = (uint16_t)(RR - SR/2);
			}
			else
			{
				r_index = (uint16_t)((RR-SR) - SR/2);
			}
		}
		else
		{
			r_index = (uint16_t)(RR - SR/2);
		}

		for(uint16_t s=0; s<size; s++)
		{
			m_index = (uint16_t)(n*size + s);
			arr->X[m_index*2] = iq_samples[(r_index+s)*2];
			arr->X[m_index*2+1] = iq_samples[(r_index+s)*2+1];
		}

		for(uint16_t m=1; m<M; m++)
		{
			for(uint16_t s=0; s<size; s++)
			{
				m_index = m*(size*N) + n*size + s;
				s_index = RR + (n*(M-1)+m-1)*size + s;
				arr->X[m_index*2] = iq_samples[s_index*2];
				arr->X[m_index*2+1] = iq_samples[s_index*2+1];
			}
		}
	}

	for (uint16_t n=0; n<N*size; n++)														//Podzielenie macierzy X na macież h i H
	{
		arr->h[n*2] = arr->X[n*2];															//W macierzy h znajdują się próbki z anteny referencyjnej
		arr->h[n*2+1] = arr->X[n*2+1];

		for (uint16_t m=1; m<M; m++)						
		{
			m_index = (m-1)*(size*N)+n;			
			n_index = m*(size*N)+n;
			arr->H[m_index*2] = arr->X[n_index*2];											//W macierzy H znajdują się próbki z pozostałych anten
			arr->H[m_index*2+1] = arr->X[n_index*2+1];
		}
	}

	for (uint16_t i=0; i<size*N*2; i++)														//Podział IQ próbek na część rzeczywistą i urojoną na potrzeby operacji matematycznych
	{
		if (i%2 == 0)
		{
			arr->h_r[i/2] = arr->h[i];
		}
		else
		{
			arr->h_i[(i-1)/2] = arr->h[i];
		}
	}

	arm_mat_trans_f32(&mat->h_r, &mat->h_T_r);
	arm_mat_trans_f32(&mat->h_i, &mat->h_T_i);

	for (uint16_t i=0; i<size*N*2; i++)
	{
		if (i%2 == 0)
		{
			arr->conj_h_T[i] = arr->h_T_r[i/2];
		}
		else
		{
			arr->conj_h_T[i] = -arr->h_T_i[(i-1)/2];
		}
	}

	for (uint16_t i=0; i<(M-1)*size*N*2; i++)
	{
		if (i%2 == 0)
		{
			arr->H_r[i/2] = arr->H[i];
		}
		else
		{
			arr->H_i[(i-1)/2] = arr->H[i];
		}
	}

	arm_mat_trans_f32(&mat->H_r, &mat->H_T_r);
	arm_mat_trans_f32(&mat->H_i, &mat->H_T_i);

	for (uint16_t i=0; i<(M-1)*size*N*2; i++)
	{
		if (i%2 == 0)
		{
			arr->conj_H_T[i] = arr->H_T_r[i/2];
		}
		else
		{
			arr->conj_H_T[i] = -arr->H_T_i[(i-1)/2];
		}
	}

	arm_mat_cmplx_mult_f32(&mat->h, &mat->conj_H_T, &mat->h_conj_H_T);
	arm_mat_cmplx_mult_f32(&mat->h, &mat->conj_h_T, &mat->h_conj_h_T);

	arr->E[0] = 1.0f;
	arr->E[1] = 0.0f;

	for (uint16_t i=0; i<(M-1)*2; i++)														//Obliczenie propagator vector
	{
		arr->E[i+2] = arr->h_conj_H_T[i] / arr->h_conj_h_T[0];							
	}

	float x_rad = 0.0f;
	float y_rad = 0.0f;
	float arg_raw = 0.0f;
	float arg_coll = 0.0f;
	float arg_raw_mult = 0.0f;
	float arg_coll_mult = 0.0f;

	float z[2] = {0};																			//Liczba zespolona określająca offset czasowy wynikający z interwału pomiarowego
	if (SW == 1)
	{
		z[0] = 0.0f;
		z[1] = -1.0f;
	}
	else if (SW == 2)
	{
		z[0] = -1.0f;
		z[1] = 0.0f;
	}

	uint16_t angle[2] = {0};
	uint8_t index = 0;

	for (uint16_t x=0; x<=PDDA_ELEVATION_SCAN+1; x+=PDDA_SEARCH_SCAN)					//Skanowanie elewacji - co PDDA_SEARCH_SCAN - od 0 do PDDA_ELEVATION_SCAN
	{
		x_rad = x*PI/180.0f;

		for (uint16_t y=0; y<=PDDA_AZIMUTH_SCAN+1; y+=PDDA_SEARCH_SCAN)					//Skanowanie azymutu - co PDDA_SEARCH_SCAN - od 0 do PDDA_AZIMUTH_SCAN
		{
			y_rad = y*PI/180.0f;
			arg_raw_mult = arm_sin_f32(x_rad) * arm_sin_f32(y_rad);
			arg_coll_mult = arm_sin_f32(x_rad) * arm_cos_f32(y_rad);

			arr->arg_x[0] = 1.0f;
			arr->arg_x[1] = 0.0f;
			arr->arg_y[0] = 1.0f;
			arr->arg_y[1] = 0.0f;

			for (uint16_t s=1; s<S; s++)														//Obliczanie argumentów do steering vector dla danej pary kątów
			{
				arg_raw = EXP * s * arg_raw_mult;
				arg_coll = EXP * s * arg_coll_mult;

				arr->arg_x[s*2] = arm_cos_f32(arg_raw);
				arr->arg_x[s*2+1] = -arm_sin_f32(arg_raw);
				arr->arg_y[s*2] = arm_cos_f32(arg_coll);
				arr->arg_y[s*2+1] = -arm_sin_f32(arg_coll);
			}

			arm_mat_cmplx_mult_f32(&mat->arg_y, &mat->arg_x, &mat->arg_yx);

			index = 0;

			for (uint16_t i=0; i<S; i++)														//Obliczanie steering vector dla danej pary kątów
			{
				for (uint16_t j=0; j<S; j++)
				{
					if (!(__USE_MATRIX_SQUARE && ((i == 1) || (i == 2)) && ((j == 1) || (j == 2))))
					{
						arr->a[index++] = arr->arg_yx[i*S*2+j*2];
						arr->a[index++] = arr->arg_yx[i*S*2+j*2+1];
					}
				}
			}

			for (uint16_t i=0; i<M; i++)														//Normalizacja pomiaru względem anteny referencyjnej - przesunięcie czasowe zgodne z "z"
			{
				for (uint16_t j=0; j<i; j++)
				{
					arm_cmplx_mult_cmplx_f32(&arr->a[2*i], z, &arr->A[2*i], 1);

					arr->a[2*i] = arr->A[2*i];
					arr->a[2*i+1] = arr->A[2*i+1];
				}
			}

			arm_mat_cmplx_mult_f32(&mat->A, &mat->E, &mat->p);							//obliczenia mocy korelacji sygnału
			arm_cmplx_mag_f32(arr->p, arr->P, 1);

			if (pdda->max.value < arr->P[0])												//zapamiętanie największej mocy
			{
				pdda->max.value = arr->P[0];
				pdda->max.elevation = x;
				pdda->max.azimuth = y;
			}
		}
	}

	pdda->max.value = 0;
	angle[0] = pdda->max.elevation;
	angle[1] = pdda->max.azimuth;

	uint16_t offset = (uint16_t)(PDDA_SEARCH_SCAN / 2);

	for (uint16_t x=angle[0]-offset; x<angle[0]+offset; x+=PDDA_FASTUP)					//Skanowanie elewacji - co PDDA_FASTUP - w poblizu maksimum angle[0]
	{
		x_rad = x*PI/180.0f;

		for (uint16_t y=angle[1]-offset; y<angle[1]+offset; y+=PDDA_FASTUP)				//Skanowanie azymutu - co PDDA_FASTUP - w poblizu maksimum angle[1]
		{
			y_rad = y*PI/180.0f;
			arg_raw_mult = arm_sin_f32(x_rad) * arm_sin_f32(y_rad);
			arg_coll_mult = arm_sin_f32(x_rad) * arm_cos_f32(y_rad);

			arr->arg_x[0] = 1.0f;
			arr->arg_x[1] = 0.0f;
			arr->arg_y[0] = 1.0f;
			arr->arg_y[1] = 0.0f;

			for (uint16_t s=1; s<S; s++)
			{
				arg_raw = EXP * s * arg_raw_mult;
				arg_coll = EXP * s * arg_coll_mult;

				arr->arg_x[s*2] = arm_cos_f32(arg_raw);
				arr->arg_x[s*2+1] = -arm_sin_f32(arg_raw);
				arr->arg_y[s*2] = arm_cos_f32(arg_coll);
				arr->arg_y[s*2+1] = -arm_sin_f32(arg_coll);
			}

			arm_mat_cmplx_mult_f32(&mat->arg_y, &mat->arg_x, &mat->arg_yx);

			index = 0;

			for (uint16_t i=0; i<S; i++)
			{
				for (uint16_t j=0; j<S; j++)
				{
					if (!(__USE_MATRIX_SQUARE && ((i == 1) || (i == 2)) && ((j == 1) || (j == 2))))
					{
						arr->a[index++] = arr->arg_yx[i*S*2+j*2];
						arr->a[index++] = arr->arg_yx[i*S*2+j*2+1];
					}
				}
			}

			for (uint16_t i=0; i<M; i++)
			{
				for (uint16_t j=0; j<i; j++)
				{
					arm_cmplx_mult_cmplx_f32(&arr->a[2*i], z, &arr->A[2*i], 1);

					arr->a[2*i] = arr->A[2*i];
					arr->a[2*i+1] = arr->A[2*i+1];
				}
			}

			arm_mat_cmplx_mult_f32(&mat->A, &mat->E, &mat->p);
			arm_cmplx_mag_f32(arr->p, arr->P, 1);

			if (pdda->max.value < arr->P[0])
			{
				pdda->max.value = arr->P[0];
				pdda->max.elevation = x;
				pdda->max.azimuth = y;
			}
		}
	}

	pdda->elevation = pdda->max.elevation;
	pdda->azimuth = pdda->max.azimuth;

	out[0] = 0;																				//Utworzenie kwaternionu
	out[1] = pdda->elevation * AOA_DEGTORAD;
	out[2] = pdda->azimuth * AOA_DEGTORAD;
	Angles_To_Quaternion(out, pdda->quaternion);
}
```

Ostatnia funkcja, którą należy omówić to SKF_Handling(), która filtruje wyniki algorytmu - kwaternion reprezentujący obliczone kąty elewacji i azymutu. Jest to liniowa wersja filtru Kalmana.

```C
void SKF_Handling(void)
{
	skf_vector *skf = &SKF;
	skf_array *arr = &skf->array;
	skf_matrix *mat = &skf->matrix;

	skf->prediction();																		//Predykcja
	arr->X[0] = skf->p_quaternion[0];														//Kwaternion predykcji (t)
	arr->X[1] = skf->p_quaternion[1];
	arr->X[2] = skf->p_quaternion[2];
	arr->X[3] = skf->p_quaternion[3];

	memcpy(arr->COV_X, arr->COV_XY, arr->dim_p*arr->dim_p);

	arm_mat_mult_f32(&mat->MAT_A, &mat->X, &mat->XX);
	arm_mat_mult_f32(&mat->MAT_A, &mat->COV_X, &mat->MAT_A_COV_X);
	arm_mat_trans_f32(&mat->MAT_A, &mat->MAT_A_T);
	arm_mat_mult_f32(&mat->MAT_A_COV_X, &mat->MAT_A_T, &mat->COV_PRED);
	arm_mat_add_f32(&mat->COV_PRED, &mat->COV_Q, &mat->COV_XX);

	arm_mat_trans_f32(&mat->MAT_H, &mat->MAT_H_T);
	arm_mat_mult_f32(&mat->COV_XX, &mat->MAT_H_T, &mat->COV_XX_MAT_H_T);
	arm_mat_mult_f32(&mat->MAT_H, &mat->COV_XX_MAT_H_T, &mat->MAT_H_COV_XX_MAT_H_T);
	arm_mat_add_f32(&mat->MAT_H_COV_XX_MAT_H_T, &mat->COV_R, &mat->COV_MEAS);
	arm_mat_inverse_f32(&mat->COV_MEAS, &mat->COV_MEAS_INV);
	arm_mat_mult_f32(&mat->COV_XX_MAT_H_T, &mat->COV_MEAS_INV, &mat->GAIN_K);

	skf->measurement();																		//Pomiar
	arr->Y_MEAS[0] = skf->m_quaternion[0];													//Kwaternion pomiarowy (t)
	arr->Y_MEAS[1] = skf->m_quaternion[1];
	arr->Y_MEAS[2] = skf->m_quaternion[2];
	arr->Y_MEAS[3] = skf->m_quaternion[3];

	arm_mat_mult_f32(&mat->MAT_H, &mat->XX, &mat->Y);
	arm_mat_sub_f32(&mat->Y_MEAS, &mat->Y, &mat->V);
	arm_mat_mult_f32(&mat->GAIN_K, &mat->V, &mat->GAIN_K_V);
	arm_mat_add_f32(&mat->XX, &mat->GAIN_K_V, &mat->XY);
	arm_mat_mult_f32(&mat->GAIN_K, &mat->MAT_H, &mat->GAIN_K_MAT_H);
	arm_mat_sub_f32(&mat->COV_I, &mat->GAIN_K_MAT_H, &mat->COV_Y);
	arm_mat_mult_f32(&mat->COV_Y, &mat->COV_XX, &mat->COV_XY);

	skf->xy_quaternion[0] = arr->XY[0];														//Kwaternion wyjściowy (t)
	skf->xy_quaternion[1] = arr->XY[1];
	skf->xy_quaternion[2] = arr->XY[2];
	skf->xy_quaternion[3] = arr->XY[3];
	Quaternion_Normalization(skf->xy_quaternion);											

	float xy[3] = {0};
	Quaternion_To_Angles(skf->xy_quaternion, xy);
	skf->xy_elevation = xy[1] * AOA_RADTODEG;
	skf->xy_azimuth = xy[2] * AOA_RADTODEG;

	float input[4] = {0};																		//Kwaternion prawdziwy (t-1)
	input[0] = skf->quaternion[0];
	input[1] = skf->quaternion[1];
	input[2] = skf->quaternion[2];
	input[3] = skf->quaternion[3];
	Quaternion_Rotate_From_Object(input, skf->xy_quaternion, skf->quaternion);			//Obrót kwaterniona prawdziwego z czasu (t-1) o kwaternion wyjściowy z czasu (t)
	Quaternion_Normalization(skf->quaternion);												//Otrzymujemy kwaternion prawdziwy dla czasu (t)

	float out[3] = {0};
	Quaternion_To_Angles(skf->quaternion, out);											//Konwersja na kąty
	if (out[1] < 0) out[1] += 2*PI;
	if (out[2] < 0) out[2] += 2*PI;
	skf->elevation = out[1] * AOA_RADTODEG;
	skf->azimuth = out[2] * AOA_RADTODEG;
}

```

Należy dodać, że wektor stanu to kwaternion. Ciała funkcji od predykcji i pomiaru zostały omówione poniżej.

```C
static void prediction(void)
{
	skf_vector *skf = &SKF;

    for(uint8_t i=0; i<SKF_SHIFT_BUF_SIZE-1; i++)														//Bufor przesuwny przechowujący ostatnie prędkości kątowe
    {
    	skf->elevation_velocity_buffer[i] = skf->elevation_velocity_buffer[i+1];
    	skf->azimuth_velocity_buffer[i] = skf->azimuth_velocity_buffer[i+1];
    }
    skf->elevation_velocity_buffer[SKF_SHIFT_BUF_SIZE-1] = skf->xy_elevation / skf->dt;			//Obliczenie prędkości kątowych i zapisanie do bufora
    skf->azimuth_velocity_buffer[SKF_SHIFT_BUF_SIZE-1] = skf->xy_azimuth / skf->dt;

    float sum[2] = {0};
    for(uint8_t i=0; i<SKF_SHIFT_BUF_SIZE; i++)
    {
    	sum[0] += skf->elevation_velocity_buffer[i];
    	sum[1] += skf->azimuth_velocity_buffer[i];
    }

    skf->elevation_velocity = sum[0]/SKF_SHIFT_BUF_SIZE;												//Obliczenie średnich prędkości kątowych
    skf->azimuth_velocity = sum[1]/SKF_SHIFT_BUF_SIZE;

	float ksi[3] = {0};																				
	ksi[0] = 0;
	ksi[1] = skf->elevation_velocity;
	ksi[2] = skf->azimuth_velocity;

	float force[4] = {0};
	force[0] = 1;																							//Skontruowanie kwaternionu wymuszenia ze średnich prędkości kątowych
	force[1] = 0.5f*skf->dt * ksi[0];
	force[2] = 0.5f*skf->dt * ksi[1];
	force[3] = 0.5f*skf->dt * ksi[2];
	Quaternion_Normalization(force);

	float prediction[4] = {0};																			//Obrót kwaternionu z czasu (t-1) o kwaternion wymuszenia
	Quaternion_Rotate_From_Object(skf->quaternion, force, prediction);
	Quaternion_Normalization(prediction);
	float conj[4] = {0};
	Quaternion_Conjugate(skf->quaternion, conj);				
	Quaternion_Rotate_From_Space(conj, prediction, skf->p_quaternion);								//Obliczenie kwaternionu predykcji dla czasu (t) - różnica q(t-1), q(t)
	Quaternion_Normalization(skf->p_quaternion);														

	float p[3] = {0};																						//Konwersja na kąty
	Quaternion_To_Angles(skf->p_quaternion, p);
	skf->p_elevation = p[1] * AOA_RADTODEG;
	skf->p_azimuth = p[2] * AOA_RADTODEG;
}

static void measurement(void)
{
	skf_vector *skf = &SKF;

	float *quaternion = (float *)skf->arg;																//Przechwycenie kwaternionu z PDDA

	float conj[4] = {0};
	Quaternion_Conjugate(skf->quaternion, conj);
	Quaternion_Rotate_From_Space(quaternion, conj, skf->m_quaternion);								//Obliczenie kwaternionu pomiaru dla czasu (t) - różnica q(t-1), qPDDA(t)
	Quaternion_Normalization(skf->m_quaternion);

	float m[3] = {0};
	Quaternion_To_Angles(skf->m_quaternion, m);														//Konwersja na kąty
	skf->m_elevation = m[1] * AOA_RADTODEG;
	skf->m_azimuth = m[2] * AOA_RADTODEG;
}
```
