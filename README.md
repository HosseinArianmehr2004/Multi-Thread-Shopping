# توضیحات کلی

این برنامه به زبان c نوشته شده است و وظیفه آن مدیریت سفارشات و خرید از چند فروشگاه مختلف است.
این برنامه شامل چندین تابع برای ورود کاربران، خواندن لیست سفارشات، خواندن داده ها از فایل ها و دایرکتوری ها و در نهایت انجام عملیات خرید می باشد
همچنین از چندین نخ و فرآیند برای بهینه سازی عملکرد استفاده شده است.
این برنامه شامل دو فایل اصلی `server.c` و `client.c` است که ابتدا جزئیات `server.c` را شرح می دهیم.

---

---

# توضیحات server.c

## معرفی متغیر ها

- `char username[100]`: نام کاربر
- `int order_number`: سقف خرید کاربر

---

## معرفی توابع اصلی

### 1. `login`

این تابع عملیات ورود کاربر به برنامه را انجام می دهد.

### عملکرد:

1- ابتدا نام کاربر را می گیرد.

2- سپس بررسی می کند که این کاربر قبلا لاگین کرده است یا خیر.

2-1- اگر کاربر قبلا لاگین کرده باشد ، تعداد دفعات خرید او را از فایل کاربر گرفته و مجموع خرید های قبلی او را محاسبه می کند.

2-2- اگر کاربر جدید باشد یک فایل جدید برای آن کاربر در پوشه کاربران ایجاد می شود.

---

### 2. `main`

این تابع بستری را فراهم می سازد تا کاربران متعدد بتوانند همزمان لاگین کنند.

### عملکرد:

1- در هر بار اجرای حلقه `while` یک کاربر می تواند لاگین کند.

2- پس از لاگین کاربر ، `username` او در یک فایل جداگانه ذخیره می شود (تا بعدا در `client.c` از آن استفاده شود).

3- تعداد خرید های قبلی کاربر را به طور مستقیم به client.c ارسال می کند.

4- ترمینال مربوط به کاربر باز شده و حلقه `while` برای لاگین کاربر بعدی مجدد تکرار می شود.

---

---

# توضیحات client.c

## معرفی متغیر ها

- `pid_t main_pid`: شناسه فرآیند اصلی
- `bool get_score_lock`: قفل برای گرفتن score و آپدیت در فایل های محصولات

- `char username[100]`: نام کاربر
- `int order_number`: تعداد خرید های قبلی کاربر
- `float price_threshold`: سقف خرید کاربر
- `char chosen_store[2]`: بهترین فروشگاه
- `order_list order_list_items[100]`: لیست سفارشات کاربر

- `char log_file_path[1024]`: .log مسیر فایل
- `pthread_mutex_t log_file_mutex`: .log فایل mutex

- `item results[100]`: لیست سفارشاتی که پیدا شدند
- `int results_count`: results شمارنده
- `pthread_mutex_t results_array_mutex`: to put the items in the results correctly

---

---

## معرفی ساختار داده ها

### 1. `order_list`

این ساختار شامل اطلاعات مربوط به سفارشات کاربر است.

- `char name[50]`: نام محصول
- `int number`: تعداد محصول

---

### 2. `item`

این ساختار در واقع لیست خرید کاربر است که شامل اطلاعات مربوط به محصولات موجود در فروشگاه ها می باشد.

- `char name[50]`: نام محصول
- `float price`: قیمت محصول
- `float score`: امتیاز محصول
- `int entity`: موجودی محصول
- `char last_modified[100]`: تاریخ و ساعت آخرین آپدیت محصول

- `int number`: تعداد محصول (سفارش داده شده)
- `char store_number`: نام فروشگاه محصول
- `char path[200]`: مسیر فایل محصول

- `pthread_t thread_id`: نخی که این محصول را پیدا کرده است
- `pid_t process_id`: فرآیندی که نخ پیدا کننده محصول را نگه می دارد

---

### 3. `shop_cart`

این ساختار شامل اطلاعات تمام محصولات پیدا شده در فروشگاه ها است

- `item cart[300]`: لیست تمام محصولات پیدا شده
- `int count`: شمارنده محصولات

---

---

## معرفی توابع اصلی

### 1. `get_order_list`

این تابع برای جمع‌آوری لیست سفارشات و سقف خرید کاربر استفاده می‌شود.

### عملکرد:

1- از کاربر نام محصولات و تعداد آن ها را می گیرد.

2- پس از اتمام ورودی، سقف خرید را از کاربر می‌گیرد.

---

### 2. `is_in_order_list`

این تابع بررسی می‌کند که آیا یک محصول خاص در یک فروشگاه در لیست سفارشات کاربر وجود دارد یا خیر.
در واقع این تابع به تعداد تمام محصولات موجود در فروشگاه ها صدا زده می شود.

### عملکرد:

1- نام محصول را با لیست سفارشات مقایسه می‌کند.

1-1- در صورت وجود تعداد محصول را به `checking_item` می افزاید و مقدار `true` بر می گرداند.

1-2- در صورت عدم وجود `false` برمی گرداند.

---

### 3. `read_file`

این تابع در فایل `.log` اطلاعات محصولات را می نویسد و اطلاعات یک محصول را از یک فایل `.txt` می خواند.

### عملکرد:

1- مسیر فایل محصول را به عنوان ورودی می گیرد.

2- در فایل `.log` محصول در دایرکتوری category آن محصول ، اطلاعات لازم را می نویسد.

3- بررسی می کند که محصول در لیست سفارشات کاربر وجود دارد یا خیر

3-1- اگر وجود داشته باشد اطلاعات محصول شامل نام ، قیمت ، امتیاز و موجودی را از فایل می خواند و آن را به ساختار `item` اضافه می کند.

---

### 4. `create_log_file`

این تابع فایل های `.log` را ایجاد می کند.

### عملکرد:

1- یک category به عنوان ورودی می گیرد.

2- در دایرکتوری آن category یک پوشه به نام `log` ایجاد می کند.

3- به ازای هر خرید یک فایل `.log` در آن پوشه ایجاد می کند.

---

### 5. `create_thread`

این تابع برای هر فایل محصول یک نخ جدید ایجاد می کند.

### عملکرد:

1- دایرکتوری مشخص شده را باز می کند و تمام فایل‌ های موجود در Category ها را می خواند.

2- برای هر فایل، یک نخ جدید ایجاد کرده و تابع `read_file` را فراخوانی می کند.

3- پس از اتمام همه نخ ها، نتایج را جمع آوری کرده و به آرایه نتایج منتقل می‌کند.

---

### 6. `create_process`

این تابع برای هر فروشگاه و دایرکتوری (در مجموع 24 دایرکتوری) یک فرآیند جدید ایجاد می کند.

### عملکرد:

1- دایرکتوری مشخص شده را باز می کند و برای هر زیر دایرکتوری یک فرآیند جدید ایجاد می کند.

2- در فرآیند فرزند، تابع `create_thread` را برای خواندن اطلاعات محصولات فراخوانی می کند و نتایج را به سبد خرید اضافه می کند.

3- از یک لوله (pipe) برای ارتباط بین فرآیند والد و فرزندان استفاده می‌کند.

4- پس از گرفتن اطلاعات سبد خرید از فرزندان ، مقدار `entity` محصولات آپدیت می شود.

---

### 7. `order`

این تابع وظیفه محاسبه ارزش هر سبد خرید و شناسایی بهترین فروشگاه بر اساس ارزش محصولات را بر عهده دارد.

### عملکرد:

1- ارزش سبد خرید را برای هر فروشگاه محاسبه می کند.

2- برای هر محصول، ارزش را بر اساس قیمت ، تعداد و امتیاز آن محاسبه می کند و در آرایه ای ذخیره می کند.

3- سپس بهترین فروشگاه را بر اساس بیشترین ارزش محاسبه شده تعیین می کند.

---

### 8. `update_files`

این تابع اطلاعات آپدیت شده محصولات خریداری شده را در فایل آن محصولات مجدد می نویسد.

### عملکرد:

1- یک محصول به عنوان ورودی می گیرد.

2- در فایل آن محصول اطلاعات را درج می کند.

---

### 9. `score`

هدف این تابع دریافت امتیاز از کاربر برای محصولات بهترین فروشگاه تعیین شده و گرفتن زمان فعلی از سیستم است.

### عملکرد:

1- از کاربر می خواهد که امتیاز محصولات را در محدوده 0 تا 5 وارد کند.

2- فقط برای محصولاتی که متعلق به فروشگاه انتخاب شده هستند، امتیاز دریافت می کند.

3- زمان فعلی را از سیستم می گیرد و برای محصولات فروشگاه انتخاب شده `score` و `last_modified` را آپدیت می کند.

4- تابع `update_files` را برای نوشتن اطلاعات آپدیت شده در فایل محصول ، صدا می زند.

---

### 10. `final`

این تابع برای نهایی کردن خرید ها طراحی شده است.

1- ابتدا فایل کاربر را برای آپدیت کردن اطلاعات خرید های کاربر باز می کند.

2- یک واحد به خرید های بهترین فروشگاه اضافه می کند.

3- اطلاعات آپدیت شده را در فایل کاربر می نویسد.

4- اگر کاربر قبلا از آن فروشگاه خریدی انجام داده باشد ، برای او 10 درصد تخفیف اعمال می کند.

### عملکرد:

---

## 11. `main`

1- ابتدا تعداد خرید های قبلی کاربر از `server.c` گرفته می شود.

2- سپس `username` کاربر از فایل مربوطه خوانده می شود.

3- با اجرای تابع `()get_order_list` کاربر می تواند لیست خرید و سقف خرید خود را وارد کند.

4- یک آرایه به نام `pipe_fd[2]` از نوع int برای ذخیره توصیف گر های لوله ایجاد می شود.

5- لوله ی `pipe(pipe_fd)` برای ارتباط بین فرآیند والد و فرزندان ایجاد می شود. اگر ایجاد لوله ناموفق باشد، خطا چاپ می شود و برنامه خاتمه می یابد.

6- با استفاده از دستور `;()main_pid = getpid` شناسه فرآیند اصلی برنامه دریافت می شود و در متغیر `main_pid` ذخیره می شود.

7- دو بار از تابع `()fork` استفاده می شود تا دو فرآیند فرزند ایجاد کند.
اگر `()fork` ناموفق باشد، پیام خطا چاپ می شود و برنامه خاتمه می یابد.

8- بر اساس اینکه کدام فرآیند فرزند در حال اجرا است، مسیر دایرکتوری فروشگاه (store_path) و نام فروشگاه تعیین می شود.

9- اگر فرآیند فرزند باشد:

9-1- با استفاده از دستور `;close(pipe_fd[0])` انتهای خواندن لوله بسته می شود.

9-2- سبد خرید (shopping_cart) و شمارنده آن (shopping_cart_count) تعریف می شود.

9-3- اطلاعات مربوط به فروشگاه پردازش می شود (با استفاده از توابع `()create_process` و `()create_thread` و `()read_file`) و سبد خرید به والد ارسال می شود.

10- اگر فرایند والد باشد:

10-1- والد منتظر می ماند تا همه فرآیند های فرزند به اتمام برسند.

10-2- پس از خاتمه فرزندان، داده‌های سبد خرید از لوله خوانده می شود و در ساختار `shop_cart` ذخیره می‌شود.

11- سپس سه نخ برای پردازش های مربوط به انتخاب بهترین سبد خرید، امتیاز دهی و پردازش نهایی ایجاد می‌شود.

12- در نهایت پس از ایجاد و اجرای نخ های نهایی ، برنامه به پایان می رسد.
