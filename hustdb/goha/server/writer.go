package server

import (
	"io"
	"strconv"
)

type Writer struct {
	w io.Writer
	b []byte
}

func NewWriter(wr io.Writer) *Writer {
	return &Writer{
		w: wr,
	}
}

func (wr *Writer) Flush() error {
	if _, err := wr.w.Write(wr.b); err != nil {
		return err
	}
	wr.b = wr.b[:0]
	return nil
}

func (wr *Writer) WriteError(msg string) {
	wr.b = append(wr.b, '-')
	wr.b = append(wr.b, msg...)
	wr.b = append(wr.b, '\r', '\n')
}

func (wr *Writer) WriteString(msg string) {
	wr.b = append(wr.b, '+')
	wr.b = append(wr.b, []byte(msg)...)
	wr.b = append(wr.b, '\r', '\n')
}

func (wr *Writer) WriteBytes(b []byte) {
	wr.b = append(wr.b, '+')
	wr.b = append(wr.b, b...)
	wr.b = append(wr.b, '\r', '\n')
}

func (wr *Writer) WriteRaw(data []byte) {
	wr.b = append(wr.b, data...)
}

func (wr *Writer) WriteInt64(num int64) {
	wr.b = append(wr.b, ':')
	wr.b = append(wr.b, []byte(strconv.FormatInt(num, 10))...)
	wr.b = append(wr.b, '\r', '\n')
}

func (wr *Writer) WriteInt(num int) {
	wr.WriteInt64(int64(num))
}

func (wr *Writer) WriteBulk(bulk []byte) {
	wr.b = append(wr.b, '$')
	wr.b = append(wr.b, strconv.FormatInt(int64(len(bulk)), 10)...)
	wr.b = append(wr.b, '\r', '\n')
	wr.b = append(wr.b, bulk...)
	wr.b = append(wr.b, '\r', '\n')
}

func (wr *Writer) WriteArray(count int) {
	wr.b = append(wr.b, '*')
	wr.b = append(wr.b, strconv.FormatInt(int64(count), 10)...)
	wr.b = append(wr.b, '\r', '\n')
}

func (wr *Writer) WriteNULL() {
	wr.b = append(wr.b, '$', '-', '1', '\r', '\n')
}
