template <typename T, typename E = fpreal>
void writeImage(const string_view _filename,
                const T* _data,
                const uinteger2 _imageDim)
{
  std::cout << "Writing image to " << _filename << '\n';
  // OpenImageIO namespace
  using namespace OIIO;
  // unique_ptr with custom deleter to close file on exit
  std::unique_ptr<ImageOutput, void (*)(ImageOutput*)> output(
    ImageOutput::create(_filename.data())
#if OIIO_VERSION >= 10900
      .release()
#endif
      ,
    [](auto ptr) {
      ptr->close();
      delete ptr;
    });
  ImageSpec is(
    _imageDim.x, _imageDim.y, sizeof(T) / sizeof(E), TypeDescMap<E>::type);
  output->open(_filename.data(), is);
  output->write_image(TypeDescMap<E>::type, _data);
}

template <typename T, typename E = fpreal>
auto readImage(const string_view _filename)
{
  // OpenImageIO namespace
  using namespace OIIO;
  // unique_ptr with custom deleter to close file on exit
  std::unique_ptr<ImageInput, void (*)(ImageInput*)> input(
    ImageInput::open(_filename.data())
#if OIIO_VERSION >= 10900
      .release()
#endif
      ,
    [](auto ptr) {
      ptr->close();
      delete ptr;
    });
  // Get the image specification and store the dimensions
  auto&& spec = input->spec();
  uinteger2 dim{spec.width, spec.height};

  // Allocated an array for our data
  auto data = std::make_unique<T[]>(dim.x * dim.y);
  // Read the data into our array, with a specified stride of how many floats
  // the user requested, i.e. for fpreal3 we ignore the alpha channel
  input->read_image(
    TypeDescMap<E>::type, data.get(), sizeof(T), AutoStride, AutoStride);

  // return an owning span
  struct OwningSpan
  {
    std::unique_ptr<T[]> m_data;
    uinteger2 m_imageDim;
  };
  return OwningSpan{std::move(data), std::move(dim)};
}
