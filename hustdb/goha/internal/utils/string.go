package utils

import "bytes"

func ConcatString(strArr ...string) string {
	var buffer bytes.Buffer
	for _, str := range strArr {
		buffer.WriteString(str)
	}

	return buffer.String()
}
