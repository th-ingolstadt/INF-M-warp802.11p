function SIGNAL_u32 = tx_signal_calc(length, mod_order, code_rate)

switch(sprintf('%d %d', mod_order, code_rate))
    case '2 0'
        RATE = uint8(10); %0101 -> 1010
    case '2 1'
        RATE = uint8(14); %0111 -> 1110
    otherwise
        error('Invalid mod_order or code_rate');
end

length_u = bitand(uint16(length), hex2dec('fff'));

length_2to0 = uint8(bitand(length_u, 7));
length_10to3 = uint8(bitand(bitshift(length_u, -3), 15));
length_msb = uint8(bitshift(length_u, -11));

parity = mod(sum(sum(dec2bin([uint32(length_u) uint32(RATE)]) == '1')), 2);

b0 = bitor(RATE, bitshift(length_2to0, 5));
b1 = length_10to3;
b2 = bitor(length_msb, bitshift(parity, 1));

SIGNAL_u32 = uint32(b0) + uint32(b1)*2^8 + uint32(b2)*2^16;
end
