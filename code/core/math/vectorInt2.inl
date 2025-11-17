String VectorInt2::to_string() const
{
	return String::printf(TXT("x:%i y:%i"), x, y);
}

String VectorInt2::to_loadable_string() const
{
	return String::printf(TXT("%i %i"), x, y);
}

VectorInt3 VectorInt2::to_vector_int3(int _z) const
{
	return VectorInt3(x, y, _z);
}


