package utils

const (
	HUSTDB_TABLE_SIZE = 1024
)

func LocateHashRegion(key string) int {
	return NgxHashKey(key) % HUSTDB_TABLE_SIZE
}

func NgxHashKey(key string) int {
	val := 0
	for _, c := range []byte(key) {
		val = (val + int(c))
	}
	return val
}

func ngxHash(key int, c byte) int {
	return key + int(c)
}
